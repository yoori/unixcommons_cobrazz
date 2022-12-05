#include <iostream>
#include "echo.hpp"

int
main(int argc, char **argv)
{
  CORBA::ORB_ptr orb = CORBA::ORB_init(argc, argv, ORB_NAME);

  {
    CORBA::Object_var obj = orb->resolve_initial_references("DynAnyFactory");
    DynamicAny::DynAnyFactory_var daf = DynamicAny::DynAnyFactory::_narrow(obj);

    DynamicAny::DynAny_var da;
    da = daf->create_dyn_any_from_type_code(CORBA::_tc_long);
    da->insert_long(20l);

    CORBA::Any_var a;
    a = da->to_any();
  }

  orb->destroy();
  return 0;
}
