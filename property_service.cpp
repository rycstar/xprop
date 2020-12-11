#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <new>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <syslog.h>
#include "private/bionic_futex.h"
#include "private/bionic_lock.h"
#include "private/bionic_macros.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>


#include <sys/poll.h>
#include <sys/select.h>
#include <vector>
#include <string>
#include <chrono>
#include "private/chrono_utils.h"

void start_property_service();
static void handle_property_set_fd();
static int epoll_fd = -1;
static char *app_name = NULL;
boot_clock::time_point boot_clock::now() {
  return boot_clock::time_point();
}

class SocketConnection {
 public:
  SocketConnection(int socket, const struct ucred& cred)
      : socket_(socket), cred_(cred) {}

  ~SocketConnection() {
    close(socket_);
  }

  bool RecvUint32(uint32_t* value, uint32_t* timeout_ms) {
    return RecvFully(value, sizeof(*value), timeout_ms);
  }

  bool RecvChars(char* chars, size_t size, uint32_t* timeout_ms) {
    return RecvFully(chars, size, timeout_ms);
  }

  bool RecvString(std::string* value, uint32_t* timeout_ms) {
    uint32_t len = 0;
    if (!RecvUint32(&len, timeout_ms)) {
      return false;
    }

    if (len == 0) {
      *value = "";
      return true;
    }

    // http://b/35166374: don't allow init to make arbitrarily large allocations.
    if (len > 0xffff) {
        PROP_LOG(LOG_ERR,"sys_prop: recvstring asked to read huge string: %d\n", len );
      errno = ENOMEM;
      return false;
    }

    std::vector<char> chars(len);
    if (!RecvChars(&chars[0], len, timeout_ms)) {
      return false;
    }

    *value = std::string(&chars[0], len);
    return true;
  }

  bool SendUint32(uint32_t value) {
    int result = TEMP_FAILURE_RETRY(send(socket_, &value, sizeof(value), 0));
    return result == sizeof(value);
  }

  int socket() {
    return socket_;
  }

  const struct ucred& cred() {
    return cred_;
  }

 private:
  bool PollIn(uint32_t* timeout_ms) {
    struct pollfd ufds[1];
    ufds[0].fd = socket_;
    ufds[0].events = POLLIN;
    ufds[0].revents = 0;
    while (*timeout_ms > 0) {
      Timer timer;
      int nr = poll(ufds, 1, *timeout_ms);
      uint64_t millis = timer.duration().count();
      *timeout_ms = (millis > *timeout_ms) ? 0 : *timeout_ms - millis;

      if (nr > 0) {
        return true;
      }

      if (nr == 0) {
        // Timeout
        break;
      }

      if (nr < 0 && errno != EINTR) {
          PROP_LOG(LOG_ERR,"sys_prop: error waiting for uid  send property message" );
        return false;
      } else { // errno == EINTR
        // Timer rounds milliseconds down in case of EINTR we want it to be rounded up
        // to avoid slowing init down by causing EINTR with under millisecond timeout.
        if (*timeout_ms > 0) {
          --(*timeout_ms);
        }
      }
    }

    //LOG(ERROR) << "sys_prop: timeout waiting for uid " << cred_.uid << " to send property message.";
    return false;
  }

  bool RecvFully(void* data_ptr, size_t size, uint32_t* timeout_ms) {
    size_t bytes_left = size;
    char* data = static_cast<char*>(data_ptr);
    while (*timeout_ms > 0 && bytes_left > 0) {
      if (!PollIn(timeout_ms)) {
        return false;
      }

      int result = TEMP_FAILURE_RETRY(recv(socket_, data, bytes_left, MSG_DONTWAIT));
      if (result <= 0) {
        return false;
      }

      bytes_left -= result;
      data += result;
    }

    return bytes_left == 0;
  }

  int socket_;
  struct ucred cred_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SocketConnection);
};

static int property_set_fd = -1;
bool StartsWith(const std::string& s, const char* prefix) {
  return strncmp(s.c_str(), prefix, strlen(prefix)) == 0;
}

static uint32_t PropertySetImpl(const std::string& name, const std::string& value) {
    size_t valuelen = value.size();

    if (valuelen >= PROP_VALUE_MAX) {
        PROP_LOG(LOG_ERR,"property_set(\"%s\", \"%s\") failed: value too long\n",name.c_str(), value.c_str() );
        return PROP_ERROR_INVALID_VALUE;
    }

    prop_info* pi = (prop_info*) __system_property_find(name.c_str());
    if (pi != nullptr) {
        // ro.* properties are actually "write-once".
        if (StartsWith(name, "ro.")) {
            PROP_LOG(LOG_ERR,"property_set(\"%s\", \"%s\") failed: property already set\n",name.c_str(), value.c_str() );
            return PROP_ERROR_READ_ONLY_PROPERTY;
        }

        __system_property_update(pi, value.c_str(), valuelen);
    } else {
        int rc = __system_property_add(name.c_str(), name.size(), value.c_str(), valuelen);
        if (rc < 0) {
            PROP_LOG(LOG_ERR,"property_set(\"%s\", \"%s\") failed: __system_property_add failed \n",name.c_str(), value.c_str() );
            return PROP_ERROR_SET_FAILED;
        }
    }

    // Don't write properties to disk until after we have read all default
    // properties to prevent them from being overwritten by default values.
    // 在我们读取了所有的默认属性之前，不要将属性写入磁盘，以防止它们被默认值覆盖。
    //if (persistent_properties_loaded && android::base::StartsWith(name, "persist.")) {
     //   write_persistent_property(name.c_str(), value.c_str());
   // }
   // property_changed(name, value);
    return PROP_SUCCESS;
}

uint32_t property_set(const std::string& name, const std::string& value) {
    static int debug = 0 ;
    if (!strcmp(name.c_str(),"xprop_srv_debug")){
        if (!strcmp(value.c_str(),"1")){
            debug = 1; 
        }else
            debug = 0;
    }
    if (debug == 1 && app_name)
        printf("(%s):%s=%s\n", app_name, name.c_str(),value.c_str());
    return PropertySetImpl(name, value);
}

static void handle_property_set(SocketConnection& socket,
                                const std::string& name,
                                const std::string& value,
                                bool legacy_protocol) {
      uint32_t result = property_set(name, value);
      if (!legacy_protocol) {
        socket.SendUint32(result);
      }
}


static void handle_property_set_fd() {
    static constexpr uint32_t kDefaultSocketTimeout = 2000; /* ms */

    int s = accept4(property_set_fd, nullptr, nullptr, SOCK_CLOEXEC);
    if (s == -1) {
        return;
    }

    struct ucred cr;
    socklen_t cr_size = sizeof(cr);
    if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
        close(s);
        PROP_LOG(LOG_ERR,"sys_prop: unable to get SO_PEERCRED\n");
        return;
    }

    SocketConnection socket(s, cr);
    uint32_t timeout_ms = kDefaultSocketTimeout;

    uint32_t cmd = 0;
    if (!socket.RecvUint32(&cmd, &timeout_ms)) {
        PROP_LOG(LOG_ERR,"sys_prop: error while reading command from the socket\n");
        socket.SendUint32(PROP_ERROR_READ_CMD);
        return;
    }

    switch (cmd) {
    case PROP_MSG_SETPROP: {
        char prop_name[PROP_NAME_MAX];
        char prop_value[PROP_VALUE_MAX];

        if (!socket.RecvChars(prop_name, PROP_NAME_MAX, &timeout_ms) ||
            !socket.RecvChars(prop_value, PROP_VALUE_MAX, &timeout_ms)) {
            PROP_LOG(LOG_ERR,"sys_prop(PROP_MSG_SETPROP): error while reading name/value from the socket\n");
          return;
        }

        prop_name[PROP_NAME_MAX-1] = 0;
        prop_value[PROP_VALUE_MAX-1] = 0;
        handle_property_set(socket, prop_value, prop_value, true);
        break;
      }

    case PROP_MSG_SETPROP2: {
        std::string name;
        std::string value;
        if (!socket.RecvString(&name, &timeout_ms) ||
            !socket.RecvString(&value, &timeout_ms)) {
            PROP_LOG(LOG_ERR,"sys_prop(PROP_MSG_SETPROP2): error while reading name/value from the socket\n");
          socket.SendUint32(PROP_ERROR_READ_DATA);
          return;
        }

        handle_property_set(socket, name, value, false);
        break;
      }

    default:
        PROP_LOG(LOG_ERR,"sys_prop: invalid command cmd:0x%x\n",cmd);
        socket.SendUint32(PROP_ERROR_INVALID_CMD);
        break;
    }

}
/*
 * CreateSocket - creates a Unix domain socket in ANDROID_SOCKET_DIR
 * ("/dev/socket") as dictated in init.rc. This socket is inherited by the
 * daemon. We communicate the file descriptor's value via the environment
 * variable ANDROID_SOCKET_ENV_PREFIX<name> ("ANDROID_SOCKET_foo").
 */
int CreateSocket(const char* name, int type, bool passcred, mode_t perm, uid_t uid, gid_t gid,
                 const char* socketcon) {
    int fd = 0;
    fd = (socket(PF_UNIX, type, 0));
    if (fd < 0) {
        PROP_LOG(LOG_ERR,"Failed to open socket %s\n",name);
        return -1;
    }


    struct sockaddr_un addr;
    memset(&addr, 0 , sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             name);

    if ((unlink(addr.sun_path) != 0) && (errno != ENOENT)) {
        PROP_LOG(LOG_ERR,"Failed to unlink old socket %s \n",name);
        return -1;
    }

    int ret = bind(fd, (struct sockaddr *) &addr, sizeof (addr));
    int savederrno = errno;
    if (ret) {
        errno = savederrno;
        PROP_LOG(LOG_ERR,"Failed to bind socket %s \n",name);
        goto out_unlink;
    }
    PROP_LOG(LOG_INFO,"Created socket %s \n",addr.sun_path);

    return fd;

out_unlink:
    unlink(addr.sun_path);
    return -1;
}

void register_epoll_handler(int fd, void (*fn)()) {
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = reinterpret_cast<void*>(fn);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        PROP_LOG(LOG_ERR, "epoll_ctl failed\n");
    }
}
static bool is_dir(const char* pathname) {
  struct stat info;
  if (stat(pathname, &info) == -1) {
    return false;
  }
  return S_ISDIR(info.st_mode);
}

void start_property_service() {
    char filename[128];
    int len = snprintf(filename, sizeof(filename), "%s/%s", PROP_SERVICE_PATH,
                                     PROP_SERVICE_NAME);
    if (len < 0 || len > 128) {
        return ;
    }

    if (!is_dir(PROP_SERVICE_PATH)) {
        mkdir(PROP_SERVICE_PATH, S_IRWXU | S_IXGRP | S_IXOTH);
    }
    property_set_fd = CreateSocket(filename, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
                                   false, 0666, 0, 0, nullptr);
    if (property_set_fd == -1) {
        PROP_LOG(LOG_ERR, "start_property_service socket creation failed\n");
        exit(1);
    }

    listen(property_set_fd, 8);
    
   
    register_epoll_handler(property_set_fd, handle_property_set_fd);
}


bool ReadFdToString(int fd, std::string* content) {
  content->clear();

  // Although original we had small files in mind, this code gets used for
  // very large files too, where the std::string growth heuristics might not
  // be suitable. https://code.google.com/p/android/issues/detail?id=258500.
  struct stat sb;
  if (fstat(fd, &sb) != -1 && sb.st_size > 0) {
    content->reserve(sb.st_size);
  }

  char buf[BUFSIZ];
  ssize_t n;
  while ((n = TEMP_FAILURE_RETRY(read(fd, &buf[0], sizeof(buf)))) > 0) {
    content->append(buf, n);
  }
  return (n == 0) ? true : false;
}


bool ReadFile(const std::string& path, std::string* content, std::string* err) {
    content->clear();
    *err = "";

    
    int fd = TEMP_FAILURE_RETRY(open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    if (fd == -1) {
        *err = "Unable to open '" + path + "': " + strerror(errno);
        return false;
    }

    // For security reasons, disallow world-writable
    // or group-writable files.
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        *err = "fstat failed for '" + path + "': " + strerror(errno);
        return false;
    }
#if 0
    if ((sb.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
        *err = "Skipping insecure file '" + path + "'";
        return false;
    }
#endif

    if (!ReadFdToString(fd, content)) {
        *err = "Unable to read '" + path + "': " + strerror(errno);
        return false;
    }
    return true;
}


static bool load_properties_from_file(const char *, const char *);
/*
 * Filter is used to decide which properties to load: NULL loads all keys,
 * "ro.foo.*" is a prefix match, and "ro.foo.bar" is an exact match.
 */
static void load_properties(char *data, const char *filter)
{
    char *key, *value, *eol, *sol, *tmp, *fn;
    size_t flen = 0;

    if (filter) {
        flen = strlen(filter);
    }

    sol = data;
    while ((eol = strchr(sol, '\n'))) {
        key = sol;
        *eol++ = 0;
        sol = eol;

        while (isspace(*key)) key++;
        if (*key == '#') continue;

        tmp = eol - 2;
        while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

        if (!strncmp(key, "import ", 7) && flen == 0) {
            fn = key + 7;
            while (isspace(*fn)) fn++;

            key = strchr(fn, ' ');
            if (key) {
                *key++ = 0;
                while (isspace(*key)) key++;
            }

            load_properties_from_file(fn, key);

        } else {
            value = strchr(key, '=');
            if (!value) continue;
            *value++ = 0;

            tmp = value - 2;
            while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

            while (isspace(*value)) value++;

            if (flen > 0) {
                if (filter[flen - 1] == '*') {
                    if (strncmp(key, filter, flen - 1)) continue;
                } else {
                    if (strcmp(key, filter)) continue;
                }
            }

            property_set(key, value);
        }
    }
}


// Filter is used to decide which properties to load: NULL loads all keys,
// "ro.foo.*" is a prefix match, and "ro.foo.bar" is an exact match.
static bool load_properties_from_file(const char* filename, const char* filter) {
    Timer t;
    std::string data;
    std::string err;
    if (!ReadFile(filename, &data, &err)) {
        //PLOG(WARNING) << "Couldn't load property file: " << err;
        printf("%s:%d  %s\n",__FUNCTION__, __LINE__, err.c_str());
        return false;
    }
    data.push_back('\n');
    load_properties(&data[0], filter);
    //LOG(VERBOSE) << "(Loading properties from " << filename << " took " << t << ".)";
    return true;
}


int main(int argc, char **argv){
    app_name = strdup(argv[0]);
    openlog(argv[0], LOG_CONS | LOG_PID, 0);
    if (__system_property_area_init()){
        printf("%s:%d __system_property_area_init failed\n",__FUNCTION__, __LINE__);
    }
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        PROP_LOG(LOG_ERR,"epoll_create1 failed\n");
        exit(1);
    }
    if(!load_properties_from_file(DEFAULT_PROPERTY,NULL))
        exit(1);
    property_set("ro.author", "terry.rong");
    start_property_service();

    while (true) {
        epoll_event ev;
        int nr = TEMP_FAILURE_RETRY(epoll_wait(epoll_fd, &ev, 1, -1 /*epoll_timeout_ms*/));
        if (nr == -1) {
            PROP_LOG(LOG_ERR,"epoll_wait failed\n");
        } else if (nr == 1) {
            ((void (*)()) ev.data.ptr)();
        }
    }
    closelog();
    return 0;
}


