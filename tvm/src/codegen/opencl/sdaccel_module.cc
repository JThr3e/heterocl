/*!
 *  Copyright (c) 2018 by Contributors
 * \file build_vhls.cc
 * \brief Build HLS C modules from source.
 */
#include "./sdaccel_module.h"
#include <fstream>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>

namespace TVM {
namespace runtime {

namespace {


void PrintIndent(std::ofstream& stream, int indent) {
  for (int i = 0; i < indent; i++)
    stream << ' ';
}



inline size_t GetTypeSize(TVMType t) {
  size_t byte = (t.bits + 7) / 8;
  if (byte > 2){
    if (byte <= 4) byte = 4;
    else if (byte <= 8) byte = 8;
    else byte = 16;
  }
  return byte;
}



inline size_t GetDataSize(TVMArray* arr) {
  size_t size = 1;
  for (tvm_index_t i = 0; i < arr->ndim; ++i) {
    size *= arr->shape[i];
  }
  size_t byte = (arr->dtype.bits + 7) / 8;
  if (byte > 2){
    if (byte <= 4) byte = 4;
    else if (byte <= 8) byte = 8;
    else byte = 16;
  }
  size *= (byte * 8 * arr->dtype.lanes + 7) / 8;
  return size;
}



inline TVMType Type2TVMType(Type t) {
  TVMType tt;
  if (t.is_int())        tt.code = kDLInt;
  else if (t.is_uint())  tt.code = kDLUInt;
  else if (t.is_float()) tt.code = kDLFloat;
  else                   LOG(FATAL) << "Unacceptable type: " << t;
  tt.bits = static_cast<uint8_t>(t.bits());
  tt.fracs = static_cast<uint8_t>(t.fracs());
  return tt;
}

// inline std::string Type2Str(TVMType t) {
//   std::string str = "";
//   if (t.code == kDLInt) {
//     if (t.fracs > 0) str += "ap_fixed<";
//     else             str += "ap_int<";
//     str += std::to_string(static_cast<int>(t.bits));
//     if (t.fracs > 0) str += ", " + std::to_string(static_cast<int>(t.bits - t.fracs)) + ">";
//     else             str += ">";
//   } else if (t.code == kDLUInt) {
//     if (t.fracs > 0) str += "ap_ufixed<";
//     else             str += "ap_uint<";
//     str += std::to_string(static_cast<int>(t.bits));
//     if (t.fracs > 0) str += ", " + std::to_string(static_cast<int>(t.bits - t.fracs)) + ">";
//     else             str += ">";
//   } else if (t.code == kDLFloat) {
//     str += "float";
//   } else {
//     LOG(FATAL) << "Unknown type";
//   }
//   return str;
// }

inline std::string Type2Str(TVMType t) {
  std::string = "";
  if (t.code == kDLInt) {
    str += "int";
  } else if (t.code == kDLInt) {
    str += "unsigned int";
  } else if (t.code == kDLFloat) {
    str += "float";
  }
  else {
    LOG(FATAL) << "Unknown type";
  }
  return str;
}



inline std::string Type2ExtStr(TVMType t) {
  std::string str = "";
  if (t.code == kDLInt) {
    if (t.fracs > 0) str += "ap_fixed<";
    else             str += "ap_int<";
    str += std::to_string(static_cast<int>(t.bits + t.fracs));
    if (t.fracs > 0) str += ", " + std::to_string(static_cast<int>(t.bits)) + ">";
    else             str += ">";
  } else if (t.code == kDLUInt) {
    if (t.fracs > 0) str += "ap_ufixed<";
    else             str += "ap_uint<";
    str += std::to_string(static_cast<int>(t.bits + t.fracs));
    if (t.fracs > 0) str += ", " + std::to_string(static_cast<int>(t.bits)) + ">";
    else             str += ">";
  } else if (t.code == kDLFloat) {
    str += "float";
  } else {
    LOG(FATAL) << "Unknown type";
  }
  return str;
}

inline std::string Type2Byte(TVMType t) {
  std::string str = "";
  if (t.code == kDLFloat) {
    str += "float";
  } else if (t.code == kDLInt || t.code == kDLUInt) {
    if (t.code == kDLUInt) str += "u";
    str += "int";
    if      (t.bits <= 8)  str += "8";
    else if (t.bits <= 16) str += "16";
    else if (t.bits <= 32) str += "32";
    else                   str += "64";
    str += "_t";
  }
  return str;
}

void CollectArgInfo(TVMArgs& args, 
                    LoweredFunc func,
                    std::vector<size_t>& arg_sizes,
                    std::vector<TVMType>& arg_types) {
  for (int i = 0; i < args.size(); i++) {
    if (args[i].type_code() == kArrayHandle) {
      TVMArray* arr = args[i];
      arg_sizes.push_back(GetDataSize(arr));
      arg_types.push_back(arr->dtype);
    } else {
      const Variable* var = func->api_args[i].as<Variable>();
      TVMType t = Type2TVMType(var->type);
      arg_sizes.push_back(GetTypeSize(t));
      arg_types.push_back(t);
    }
  }
}

void GenSharedMem(TVMArgs& args,
                  std::vector<int>& shmids,
                  std::vector<size_t>& arg_sizes) {
  for (int i = 0; i < args.size(); i++) {
    if (args[i].type_code() == kArrayHandle) {
      TVMArray* arr = args[i];
      // generate shared memory key and id
      // TODO: maybe get the current path??
      key_t key = ftok("/", i+1);
      int shmid = shmget(key, arg_sizes[i], 0666|IPC_CREAT);
      shmids.push_back(shmid);
      // copy mem from TVM args to the shared memory
      void* mem = shmat(shmid, nullptr, 0);
      memcpy(mem, arr->data, arg_sizes[i]);
    } else {
      shmids.push_back(0);
    }
  }
}

void FreeSharedMem(TVMArgs& args, 
                   const std::vector<int>& shmids,
                   std::vector<size_t>& arg_sizes) {
  for (size_t i = 0; i < shmids.size(); i++) {
    if (args[i].type_code() == kArrayHandle) {
      TVMArray* arr = args[i];
      int shmid = shmids[i];
      void* mem = shmat(shmid, nullptr, 0);
      memcpy(arr->data, mem, arg_sizes[i]);
      shmdt(mem);
      shmctl(shmid, IPC_RMID, nullptr);
    }
  }
}

// copy values from the shared mem to local mem
void PrintCopy(TVMArray* arr, 
               std::ofstream& stream, 
               int indent, size_t nth_arr) {
  for (int i = 0; i < arr->ndim; i++) {
    PrintIndent(stream, indent);
    stream << "for (size_t i" << i << " = 0; ";
    stream << "i" << i << " < " << arr->shape[i] << "; ";
    stream << "i" << i << "++) {\n";
    indent += 2;
    if (i == arr->ndim-1) {
      PrintIndent(stream, indent);
      stream << "arg_top_" << nth_arr;
      for (int j = 0; j < arr->ndim; j++) {
        stream << "[i" << j << "]"; 
      }
      stream << " = (";
      stream << Type2ExtStr(arr->dtype);
      stream << ")(arg_" << nth_arr;
      stream << "[i" << arr->ndim-1;
      int mul = 1;
      for (int j = arr->ndim-2; j >= 0; j--) {
        mul *= arr->shape[j+1];
        stream << " + i" << j << "*" << mul;
      }
      stream << "])";
      if (arr->dtype.fracs > 0)
        stream << " >> " << static_cast<int>(arr->dtype.fracs);
      stream << ";\n";
    }
  }
  for (int i = 0; i < arr->ndim; i++) {
    indent -= 2;
    PrintIndent(stream, indent);
    stream << "}\n";
  }
}

// copy values from local mem back to shared mem
void PrintCopyBack(TVMArray* arr, 
                   std::ofstream& stream, 
                   int indent, size_t nth_arr) {
  for (int i = 0; i < arr->ndim; i++) {
    PrintIndent(stream, indent);
    stream << "for (size_t i" << i << " = 0; ";
    stream << "i" << i << " < " << arr->shape[i] << "; ";
    stream << "i" << i << "++) {\n";
    indent += 2;
    if (i == arr->ndim-1) {
      PrintIndent(stream, indent);
      stream << "arg_" << nth_arr;
      stream << "[i" << arr->ndim-1;
      int mul = 1;
      for (int j = arr->ndim-2; j >= 0; j--) {
        mul *= arr->shape[j+1];
        stream << " + i" << j << "*" << mul;
      }
      stream << "] = (";
      stream << Type2ExtStr(arr->dtype);
      stream << ")(arg_top_" << nth_arr;
      for (int j = 0; j < arr->ndim; j++) {
        stream << "[i" << j << "]"; 
      }
      stream << ")";
      if (arr->dtype.fracs > 0)
        stream << " << " << static_cast<int>(arr->dtype.fracs);
      stream << ";\n";
    }
  }
  for (int i = 0; i < arr->ndim; i++) {
    indent -= 2;
    PrintIndent(stream, indent);
    stream << "}\n";
  }
}

void GenHostCode(TVMArgs& args,
                 const std::vector<int>& shmids,
                 const std::vector<TVMType>& arg_types,
                 LoweredFunc func,
                 std::string test_file) {
  int indent = 0;
  std::ofstream stream;
  stream.open("main.cpp");


  stream << "#define CL_HPP_CL_1_2_DEFAULT_BUILD\n";
  stream << "#define CL_HPP_TARGET_OPENCL_VERSION 120\n";
  stream << "#define CL_HPP_MINIMUM_OPENCL_VERSION 120\n";
  stream << "#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1\n";
  stream << "#include <CL/cl2.hpp>\n";
  stream << "#include <fstream>\n";
  stream << "#include <sys/types.h>\n";
  stream << "#include <sys/stat.h>\n";
  stream << "#include <fcntl.h>\n";
  stream << "#include <unistd.h>\n";
  stream << "#include <stdlib.h>\n";
  stream << "#include <stdio.h>\n";
  stream << "#include <cstring>\n";
  stream << "#include <iostream>\n";
  stream << "#include <iomanip>\n";
  stream << "#include <math.h>\n";
  stream << "#pragma once\n";
  stream << "\n\n";
  
  // stream << test_file;

  stream << "int main(void) { \n";
  stream << "#if defined(SDX_PLATFORM) && !defined(TARGET_DEVICE)\n";
  indent += 2;
  stream << "  #define STR_VALUE(arg) #arg\n";
  stream << "  #define GET_STRING(name) STR_VALUE(name)\n";
  stream << "  #define TARGET_DEVICE GET_STRING(SDX_PLATFORM)\n";
  stream << "#endif\n";

  // get the krnl code
  PrintIndent(stream, indent);
  stream << "char* xclbinFilename = argv[1];\n";


  // Source Memories
  // std::vector<unsigned int> source_a(LENGTH);








  // Getting First Platform
  PrintIndent(stream, indent);
  stream << "std::vector<cl::Platform> platforms;\n";
  PrintIndent(stream, indent);
  stream << "cl::Platform::get(&platforms);\n";
  PrintIndent(stream, indent);
  stream << "cl::Platform platform = platforms[0];\n";


  // Getting ACCELERATOR Devices and selecting 1st such device
  PrintIndent(stream, indent);
  stream << "std::vector<cl::Device> devices;\n";
  PrintIndent(stream, indent);
  stream << "platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);\n";
  PrintIndent(stream, indent);
  stream << "cl::Device device = devices[0];\n";

  // Creating Context and Command Queue for selected Device
  PrintIndent(stream, indent);
  stream << "cl::Context context(device);\n";
  PrintIndent(stream, indent);
  stream << "cl::CommandQueue q(context, device);\n";

  // Loading XCL Bin into char buffer
  stream << "std::ifstream bin_file(xclbinFilename, std::ifstream::binary);\n";
  stream << "bin_file.seekg (0, bin_file.end);\n";
  stream << "unsigned nb = bin_file.tellg();\n";
  stream << "bin_file.seekg (0, bin_file.beg);\n";
  stream << "char *buf = new char [nb];\n";
  stream << "bin_file.read(buf, nb);\n";


  // Creating Program from Binary File
  stream << "cl::Program::Binaries bins;\n";
  stream << "bins.push_back({buf,nb});\n";
  stream << "devices.resize(1);\n";
  stream << "cl::Program program(context, devices, bins);\n";


  // Creating Kernel and Functor of Kernel
  stream << "int err1;\n";
  stream << "cl::Kernel kernel(program, \"default_function\", &err1);\n";
  stream << "auto default_function = cl::KernelFunctor<cl::Buffer&, cl::Buffer&, cl::Buffer&>(kernel);\n";



  // Creating Buffers inside Device
  // cl::Buffer buffer_a(context, CL_MEM_READ_ONLY,  vector_size_bytes);
  // cl::Buffer buffer_b(context, CL_MEM_WRITE_ONLY, vector_size_bytes);





  // Copying input data to Device buffer from host memory
  // q.enqueueWriteBuffer(buffer_a, CL_TRUE, 0, vector_size_bytes, source_a.data());





  // Running Kernel
  PrintIndent(stream, indent);
  stream << func->name << "(";
  stream << "cl::EnqueueArgs(q, cl::NDRange(1,1,1), cl::NDRange(1,1,1)),";
  for (int i = 0; i < args.size(); i++) {
    stream << "arg_top_" << i;
    if (i != args.size()-1) 
      stream << ", ";
  }
  stream << ");\n";

  PrintIndent(stream, indent);
  stream << "q.finish()\n";


  // Copying Device result data to Host memory
  // q.enqueueReadBuffer(buffer_c, CL_TRUE, 0, vector_size_bytes, result_krnl.data());






  for (int i = 0;i < args.size();i++) {
    PrintIndent(stream, indent);
    // stream << Type2Str(arg_types[i]) << " ";
    stream << arg_types[i] << " ";
    stream << "arg_" << i;
    TVMArray* arr = args[i];
    for (int j = 0;j < arr->ndim;j++) {
      stream << "[" << arr->shape[j] << "]";
    }
    stream << ";\n";
  }







  // for (int i = 0; i < args.size(); i++) {
  //   if (args[i].type_code() == kArrayHandle) {
  //     // read from the shared memory
  //     PrintIndent(stream, indent);
  //     stream << Type2Byte(arg_types[i]) << "* "; 
  //     stream << "arg_" << i << " = ";
  //     stream << "(" << Type2Byte(arg_types[i]) << "*)";
  //     stream << "shmat(" << shmids[i] << ", nullptr, 0);\n";
  //     PrintIndent(stream, indent);
  //     stream << Type2Str(arg_types[i]) << " ";
  //     stream << "arg_top_" << i;
  //     TVMArray* arr = args[i];
  //     for (int j = 0; j < arr->ndim; j++)
  //       stream << "[" << arr->shape[j] << "]";
  //     stream << ";\n";
  //     // copy from shared mem
  //     PrintCopy(arr, stream, indent, i);
  //   } else {
  //     // directly assign the value to the variable
  //     PrintIndent(stream, indent);
  //     stream << Type2Byte(arg_types[i]) << " ";
  //     stream << "arg_" << i << " = ";
  //     stream << "(" << Type2Byte(arg_types[i]) << ")";
  //     if (args[i].type_code() == kDLInt || 
  //         args[i].type_code() == kDLUInt) {
  //       stream << int64_t(args[i]);
  //     }
  //     stream << ";\n";
  //     PrintIndent(stream, indent);
  //     stream << Type2Str(arg_types[i]) << " ";
  //     stream << "arg_top_" << i;
  //     stream << " = (";
  //     stream << Type2ExtStr(arg_types[i]);
  //     stream << ")(arg_" << i << ")";
  //     if (arg_types[i].fracs > 0)
  //       stream << " >> " << static_cast<int>(arg_types[i].fracs);
  //     stream << ";\n";
  //   }
  // }


  // // call the function
  // PrintIndent(stream, indent);
  // stream << func->name << "(";
  // for (int i = 0; i < args.size(); i++) {
  //   stream << "arg_top_" << i;
  //   if (i != args.size()-1) 
  //     stream << ", ";
  // }
  // stream << ");\n";



  // // copy to shared mem
  // for (int i = 0; i < args.size(); i++) {
  //   if (args[i].type_code() == kArrayHandle) {
  //     TVMArray* arr = args[i];
  //     PrintCopyBack(arr, stream, indent, i);
  //     PrintIndent(stream, indent);
  //     stream << "shmdt(";
  //     stream << "arg_" << i << ");\n";
  //   }
  // }

  stream << "}\n";
  stream.close();
}
} // namespace



class SDAccelModuleNode final : public ModuleNode {
 public:
  SDAccelModuleNode(LoweredFunc func, std::string test_file) 
    : func_(func), test_file_(test_file) {}

  const char* type_key() const {
    return "sdaccel_sw_emu";

  }

  PackedFunc GetFunction(
      const std::string& name,
      const std::shared_ptr<ModuleNode>& sptr_to_self) final {
    return PackedFunc([this](TVMArgs args, TVMRetValue* rv){
        if (args.size() != (int)func_->args.size())
          LOG(FATAL) << "The function should take in " << func_->args.size() 
                     << " inputs but get " << args.size();
        std::vector<size_t> arg_sizes;
        std::vector<TVMType> arg_types;
        std::vector<int> shmids;
        CollectArgInfo(args, func_, arg_sizes, arg_types);
        GenSharedMem(args, shmids, arg_sizes);
        GenHostCode(args, shmids, arg_types, func_, test_file_);
        // TODO: find a better way to do the following
        LOG(CLEAN) << "Compiling the generated SDAccel OpenCL Code ...";
        system("make -f sdaccel.mk run_cpu_em");
        LOG(CLEAN) << "Running SDAccel OpenCL Software Simulation ...";
        LOG(CLEAN) << "Finished SDAccel OpenCL Software Simulation ...";
        system("make -f sdaccel.mk cleanall");
        FreeSharedMem(args, shmids, arg_sizes);
      });
  }

 private:
  LoweredFunc func_;
  std::string test_file_;
};

Module CreateSDAccelModule(
    LoweredFunc func,
    std::string code) {

  std::shared_ptr<SDAccelModuleNode> n =
    std::make_shared<SDAccelModuleNode>(func, code);

  return Module(n);
}

} // namespace runtime
} // namespace TVM