
// To compile on Linux: gcc --shared -fPIC -o bendinfix.pd_linux bendinfix.c

#include <m_pd.h>

static t_class *bendinfix_class;
static int *legacy, *legacy_bendin;
static void (*nw_gui_vmess)(const char *sel, char *fmt, ...);

typedef struct _bendinfix {
  t_object  x_obj;
} t_bendinfix;

void bendinfix_float(t_bendinfix *x, t_floatarg f)
{
  // vanilla default:
  t_float g = 8192;
  // exported symbols by the different Pd flavors:
  // nw_gui_vmess => purr-data only
  // legacy => pd-l2ork and purr-data
  // legacy_bendin => purr-data with revised bendin implementation
  if (legacy_bendin)
    // signed bendin unless legacy_bendin is set
    g = *legacy_bendin?8192:0;
  else if (legacy)
    // we always have a signed bendin with classic pd-l2ork (!nw_gui_vmess),
    // whereas for purr-data without the revised bendin implementation
    // (!legacy_bendin) bendin is signed, unless legacy is set
    g = !nw_gui_vmess?0:*legacy?8192:0;
  outlet_float(x->x_obj.ob_outlet, f-g);
}

void *bendinfix_new(void)
{
  t_bendinfix *x = (t_bendinfix *)pd_new(bendinfix_class);
  outlet_new(&x->x_obj, &s_float);
  return (void *)x;
}

#ifdef WIN32
#include <windows.h>
#else
#define __USE_GNU // to get RTLD_DEFAULT
#include <dlfcn.h> // for dlsym
#ifndef RTLD_DEFAULT
/* If RTLD_DEFAULT still isn't defined then just passing NULL will hopefully
   do the trick. */
#define RTLD_DEFAULT NULL
#endif
#endif

void bendinfix_setup(void) {
  bendinfix_class = class_new(gensym("bendinfix"),
        (t_newmethod)bendinfix_new,
        0, sizeof(t_bendinfix),
        CLASS_DEFAULT, 0);
  class_addfloat(bendinfix_class, bendinfix_float);
#ifdef WIN32
  legacy = (void*)GetProcAddress(GetModuleHandle("pd.dll"), "sys_legacy");
  legacy_bendin = (void*)GetProcAddress(GetModuleHandle("pd.dll"), "sys_legacy_bendin");
  nw_gui_vmess = (void*)GetProcAddress(GetModuleHandle("pd.dll"), "gui_vmess");
#else
  legacy = dlsym(RTLD_DEFAULT, "sys_legacy");
  legacy_bendin = dlsym(RTLD_DEFAULT, "sys_legacy_bendin");
  nw_gui_vmess = dlsym(RTLD_DEFAULT, "gui_vmess");
#endif
}
