from cffi import FFI
import os

ffibuilder = FFI()
ffibuilder.set_source("libpyrtcdcpp",
                      """
                      typedef struct PeerConnection PeerConnection;
                      typedef struct DataChannel DataChannel;
                      #include <stdbool.h>
                      #include <glib.h>
                      #include <sys/types.h>
                      #include "rtcdcpp/librtcdcpp.h"
                      """,
                      include_dirs=["../src/", "../include",
                                    "/usr/include/glib-2.0",
                                    "/usr/lib64/glib-2.0/include",
                                    "/usr/lib/glib-2.0/include",
                                    "/usr/lib/arm-linux-gnueabihf/glib-2.0/include"],
                      libraries=["rtcdcpp"],
                      library_dirs=["../"])

lines = open(os.path.abspath("../include/rtcdcpp") + "/librtcdcpp.h")
altered_lines = ['' if line.startswith('#include ') or
                 line.startswith('#pragma') or line.startswith('extern') or
                 line.startswith('typedef struct rtcdcpp::') or
                 line.startswith('#if') or line.startswith('#e')
                 else line for line in lines]

cdef_string = '''
typedef char... gchar;
typedef unsigned int guint;
struct GArray {
    gchar *data;
    guint len;
};
typedef struct GArray GArray;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint8_t u_int8_t;
typedef bool gboolean;
typedef const void *gconstpointer;

typedef unsigned int pid_t;

GArray *
g_array_new (gboolean zero_terminated,
             gboolean clear_,
             guint element_size);
GArray * g_array_append_vals(GArray* a, gconstpointer v, guint len);
''' + ''.join(altered_lines)

cdef_string = cdef_string[:-2]  # Remove the trailing }

cdef_string += '''
extern "Python" void onDCCallback(int pid, void* socket,
cb_event_loop* cb_loop);
extern "Python" void onIceCallback(IceCandidate_C ice);

extern "Python" void onOpened(void);
extern "Python" void onClosed(int pid);
extern "Python" void onStringMsg(int pid, const char* message);
extern "Python" void onBinaryMsg(void* message);
extern "Python" void onError(const char* description);

'''

ffibuilder.cdef(cdef_string)


if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
