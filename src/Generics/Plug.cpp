#include <cstdlib>


extern "C"
{
#define FUNC(x) \
void \
x(void) \
{ \
  std::abort(); \
}

  FUNC(boot_DynaLoader)

  FUNC(perl_parse)
  FUNC(Perl_newXS)
  FUNC(perl_alloc)
  FUNC(perl_free)
  FUNC(perl_run)
  FUNC(Perl_eval_pv)
  FUNC(perl_destruct)
  FUNC(perl_construct)

  FUNC(rpmReadConfigFiles)
  FUNC(rpmGetPath)
  FUNC(rpmdbOpen)
  FUNC(rpmdbClose)
  FUNC(rpmdbInitIterator)
  FUNC(rpmdbFreeIterator)
  FUNC(rpmdbNextIterator)
  FUNC(rpmdbGetIteratorOffset)

  FUNC(sensors_init)
  FUNC(sensors_get_feature)
  FUNC(sensors_get_all_features)
  FUNC(sensors_get_label)
  FUNC(sensors_get_detected_chips)

  FUNC(hosts_ctl)
}
