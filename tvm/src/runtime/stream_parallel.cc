#include <iostream>
#include <tvm/runtime/c_runtime_api.h>
#include <tvm/runtime/c_backend_api.h>

int TestExternalFunc(){
    std::cerr << "Hello!\n";
}
