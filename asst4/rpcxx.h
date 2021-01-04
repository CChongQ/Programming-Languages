// -*- c++ -*-
#ifndef RPCXX_H
#define RPCXX_H

#include <cstdlib>
#include "rpc.h"
#include <iostream>
#include <typeinfo>

namespace rpc {

// Protocol is used for encode and decode a type to/from the network.
//
// You may use network byte order, but it's optional. We won't test your code
// on two different architectures.

// TASK1: add more specializations to Protocol template class to support more
// types.
template <typename T> 
struct Protocol {
  static constexpr size_t TYPE_SIZE = sizeof(T);

  /* out_bytes: Write data into this buffer. It's size is equal to *out_len
   *   out_len: Initially, *out_len tells you the size of the buffer out_bytes.
   *            However, it is also used as a return value, so you must set *out_len
   *            to the number of bytes you wrote to the buffer.
   *         x: the data you want to write to buffer
   */ 
  static bool Encode(uint8_t *out_bytes, uint32_t *out_len, const T &x) {
	// check if buffer is big enough to fit the data, if not, return false
    if (*out_len < TYPE_SIZE) return false; 
	
	// do a memory copy of the data into the buffer, TYPE_SIZE is the size of the data
    memcpy(out_bytes, &x, TYPE_SIZE);
	
	// since we wrote TYPE_SIZE number of bytes to the buffer, we set *out_len to TYPE_SIZE
    *out_len = TYPE_SIZE;

    return true;
  }

  /* in_bytes: Read data from this buffer. It's size is equal to *in_len
   *   in_len: Initially, *in_len tells you the size of the buffer in_bytes.
   *           However, it is also used as a return value, so you must set *in_len
   *           to the number of bytes you consume from the buffer.
   *        x: the data you want to read from the buffer
   */ 
  static bool Decode(uint8_t *in_bytes, uint32_t *in_len, bool *ok, T &x) {
	// check if buffer is big enough to read in x, if not, return false
    if (*in_len < TYPE_SIZE) return false;
	
	// do a memory copy from the buffer into the data, TYPE_SIZE is the size of the data
    memcpy(&x, in_bytes, TYPE_SIZE);
	
	// since we consumed TYPE_SIZE number of bytes from the buffer, we set *in_len to TYPE_SIZE
    *in_len = TYPE_SIZE;
	
    return true;
  }
};

template <> struct Protocol<std::string> {
  static constexpr size_t LENGTH_SIZE = sizeof(size_t);

  static bool Encode(uint8_t *out_bytes, uint32_t *out_len, const std::string &x) {
    size_t STRING_SIZE = x.length();
    if (*out_len < STRING_SIZE + LENGTH_SIZE) return false;
    memcpy(out_bytes, &STRING_SIZE, LENGTH_SIZE);

    const char* x_ptr = &x[0];
    memcpy(out_bytes+LENGTH_SIZE, x_ptr, STRING_SIZE);
    *out_len = STRING_SIZE + LENGTH_SIZE;
    return true;
  }
  
  static bool Decode(uint8_t *in_bytes, uint32_t *in_len, bool *ok, std::string &x) {
    if (*in_len < LENGTH_SIZE) return false;
    size_t STRING_SIZE;
    memcpy(&STRING_SIZE, in_bytes, LENGTH_SIZE);

    if (*in_len < STRING_SIZE + LENGTH_SIZE) return false;
    // char* x_ptr = new char[STRING_SIZE];
    // memcpy(x_ptr, in_bytes+LENGTH_SIZE, STRING_SIZE);
    
    // std::string x_str (x_ptr);
    // x.assign(x_str);

    x = std::string(reinterpret_cast<const char *>(in_bytes + LENGTH_SIZE), STRING_SIZE);


    *in_len = STRING_SIZE + LENGTH_SIZE;
    return true;
  }
};


/*-------------------------------------------------------------------------------*/

// TASK2: Client-side
class IntParam : public BaseParams {
  int p;
 public:
  IntParam(int p) : p(p) {}

  bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
    return Protocol<int>::Encode(out_bytes, out_len, p);
  }
};

class VoidParam : public BaseParams {
 public:
  VoidParam() {}

  bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
    *out_len = 0;
    return true;
  }
};

class UnIntParam : public BaseParams {
  unsigned int p;
 public:
  UnIntParam(unsigned int p) : p(p) {}

  bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
    return Protocol<unsigned int>::Encode(out_bytes, out_len, p);
  }
};

class StrParam : public BaseParams {
  std::string p;
  public:
    StrParam(std::string  p) : p(p) {}

    bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
      return Protocol<std::string>::Encode(out_bytes, out_len, p);
    }
};

class StrIntParam : public BaseParams {
  std::string p_1;
  int p_2;

 public:
  StrIntParam(std::string p_1,int p_2) : p_1(p_1),p_2(p_2) {}

  bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
    
    uint32_t temp_len = *out_len;
    uint32_t used_len = 0;

    //string
    if (!Protocol<std::string>::Encode(out_bytes, out_len, p_1)) {
      return false;
    }
    temp_len = temp_len - *out_len; // now out_len= typesize, temp_len = the remaining length
    out_bytes = out_bytes + *out_len;
    used_len = *out_len;
  
    //int
    if (!Protocol<int>::Encode(out_bytes, &temp_len, p_2)) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *out_len = used_len;

    return true;
  }
};

class IntUnIntParam : public BaseParams {
  int p_1;
  unsigned int p_2;

 public:
  IntUnIntParam(int p_1,unsigned int p_2) : p_1(p_1),p_2(p_2) {}

  bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
    
    uint32_t temp_len = *out_len;
    uint32_t used_len = 0;

    //int
    if (!Protocol<int>::Encode(out_bytes, out_len, p_1)) {
      return false;
    }
    temp_len = temp_len - *out_len; // now out_len= typesize, temp_len = the remaining length
    out_bytes = out_bytes + *out_len;
    used_len = *out_len;
  
    //unsigned int 
    if (!Protocol<unsigned int>::Encode(out_bytes, &temp_len, p_2)) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *out_len = used_len;

    return true;
  }
};

class StrStrParam : public BaseParams {
  std::string p_1;
  std::string p_2;

 public:
  StrStrParam(std::string p_1,std::string p_2) : p_1(p_1),p_2(p_2) {}

  bool Encode(uint8_t *out_bytes, uint32_t *out_len) const override {
    
    uint32_t temp_len = *out_len;
    uint32_t used_len = 0;

    //string
    if (!Protocol<std::string>::Encode(out_bytes, out_len, p_1)) {
      return false;
    }
    temp_len = temp_len - *out_len; // now out_len= typesize, temp_len = the remaining length
    out_bytes = out_bytes + *out_len;
    used_len = *out_len;
  
    //string
    if (!Protocol<std::string>::Encode(out_bytes, &temp_len, p_2)) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *out_len = used_len;

    return true;
  }
};


// TASK2: Server-side
// Sample function
template<typename Svc>
class Int_IntProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    int x;
    // This function is similar to Decode. We need to return false if buffer
    // isn't large enough, or fatal error happens during parsing.
    if (!Protocol<int>::Decode(in_bytes, in_len, ok, x) || !*ok) {
      return false;
    }
    // Now we cast the function pointer func_ptr to its original type.
    //
    // This incomplete solution only works for this type of member functions.
    using FunctionPointerType = int (Svc::*)(int);
    auto p = func_ptr.To<FunctionPointerType>();
    int result = (((Svc *) instance)->*p)(x);
    if (!Protocol<int>::Encode(out_bytes, out_len, result)) {
      // out_len should always be large enough so this branch shouldn't be
      // taken. However just in case, we return an fatal error by setting *ok
      // to false.
      *ok = false;
      return false;
    }
    return true;
  }
};

//Function (1)
template <typename Svc>
class VoidProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    *in_len = 0;

    using FunctionPointerType = void (Svc::*)();
    auto p = func_ptr.To<FunctionPointerType>();
    (((Svc *) instance)->*p)();
    *out_len = 0;
    return true;
  }
};

//Function (2)
template <typename Svc>
class BoolProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    *in_len = 0;

    using FunctionPointerType = bool (Svc::*)();
    auto p = func_ptr.To<FunctionPointerType>();
    bool result = (((Svc *) instance)->*p)();
    if (!Protocol<bool>::Encode(out_bytes, out_len, result)) {
      *ok = false;
      return false;
    }
    return true;
  }
};

//Function (3)
template <typename Svc>
class StringProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    unsigned int x;

    if (!Protocol<unsigned int>::Decode(in_bytes, in_len, ok, x) || !*ok) {
      return false;
    }

    using FunctionPointerType = std::string (Svc::*)(unsigned int);
    auto p = func_ptr.To<FunctionPointerType>();
    std::string result = (((Svc *) instance)->*p)(x);
    if (!Protocol<std::string>::Encode(out_bytes, out_len, result)) {
      *ok = false;
      return false;
    }
    return true;
  }
};

//function (4)
template <typename Svc>
class StrStrProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    std::string x;
    if (!Protocol<std::string>::Decode(in_bytes, in_len, ok, x) || !*ok) {
      return false;
    }

    using FunctionPointerType = std::string (Svc::*)(std::string);
    auto p = func_ptr.To<FunctionPointerType>();
    std::string result = (((Svc *) instance)->*p)(x);
    if (!Protocol<std::string>::Encode(out_bytes, out_len, result)) {
      *ok = false;
      return false;
    }
    return true;
  }
};

//function (5)
template <typename Svc>
class StrIntProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    std::string x_1;
    int x_2;

    uint32_t temp_len = *in_len;
    uint32_t used_len = 0;

    //string
    if (!Protocol<std::string>::Decode(in_bytes, in_len, ok, x_1) || !*ok) {
      return false;
    }
    temp_len = temp_len - *in_len; // now in_len = typesize, temp_len = the remaining length
    in_bytes = in_bytes + *in_len;
    used_len = *in_len;
  
    //int
    if (!Protocol<int>::Decode(in_bytes, &temp_len, ok, x_2) || !*ok) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *in_len = used_len;

    // Now we cast the function pointer func_ptr to its original type.
    
    using FunctionPointerType = std::string (Svc::*)(std::string, int);
    auto p = func_ptr.To<FunctionPointerType>();
    std::string result = (((Svc *) instance)->*p)(x_1,x_2);
    if (!Protocol<std::string>::Encode(out_bytes, out_len, result)) {
      *ok = false;
      return false;
    }
    return true;
  }
};

//Function (6) - 1
template <typename Svc>
class UnInt_StrIntProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    std::string x_1;
    int x_2;
    
    uint32_t temp_len = *in_len;
    uint32_t used_len = 0;

    //string
    if (!Protocol<std::string>::Decode(in_bytes, in_len, ok, x_1) || !*ok) {
      return false;
    }
    temp_len = temp_len - *in_len; // now in_len = typesize, temp_len = the remaining length
    in_bytes = in_bytes + *in_len;
    used_len = *in_len;
  
    //int
    if (!Protocol<int>::Decode(in_bytes, &temp_len, ok, x_2) || !*ok) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *in_len = used_len;

    // Now we cast the function pointer func_ptr to its original type.
    
    using FunctionPointerType = unsigned int (Svc::*)(std::string, int);
    auto p = func_ptr.To<FunctionPointerType>();
    unsigned int result = (((Svc *) instance)->*p)(x_1,x_2);
    if (!Protocol<unsigned int>::Encode(out_bytes, out_len, result)) {
      *ok = false;
      return false;
    }
    return true;
  }
};

//Function (6) - 2
template <typename Svc>
class Long_IntUnIntProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    int x_1;
    unsigned int x_2;
    
    uint32_t temp_len = *in_len;
    uint32_t used_len = 0;

    //int
    if (!Protocol<int>::Decode(in_bytes, in_len, ok, x_1) || !*ok) {
      return false;
    }
    temp_len = temp_len - *in_len; // now in_len = typesize, temp_len = the remaining length
    in_bytes = in_bytes + *in_len;
    used_len = *in_len;
  
    //unsigned int
    if (!Protocol<unsigned int>::Decode(in_bytes, &temp_len, ok, x_2) || !*ok) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *in_len = used_len;

    // Now we cast the function pointer func_ptr to its original type.
    
    using FunctionPointerType = unsigned long (Svc::*)(int, unsigned int);
    auto p = func_ptr.To<FunctionPointerType>();
    unsigned long result = (((Svc *) instance)->*p)(x_1,x_2);
    if (!Protocol<unsigned long>::Encode(out_bytes, out_len, result)) {
      *ok = false;
      return false;
    }
    return true;
  }
};

//function (7) - 1
template <typename Svc>
class Void_StrIntProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    std::string x_1;
    int x_2;
    
    uint32_t temp_len = *in_len;
    uint32_t used_len = 0;

    //string
    if (!Protocol<std::string>::Decode(in_bytes, in_len, ok, x_1) || !*ok) {
      return false;
    }
    temp_len = temp_len - *in_len; // now in_len = typesize, temp_len = the remaining length
    in_bytes = in_bytes + *in_len;
    used_len = *in_len;
  
    //int
    if (!Protocol<int>::Decode(in_bytes, &temp_len, ok, x_2) || !*ok) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *in_len = used_len;

    // Now we cast the function pointer func_ptr to its original type.
    
    using FunctionPointerType = void (Svc::*)(std::string, int);
    auto p = func_ptr.To<FunctionPointerType>();
    (((Svc *) instance)->*p)(x_1,x_2);
    *out_len = 0;
    return true;
  }
};

//Function (7) - 2
template <typename Svc>
class Void_StrStrProcedure : public BaseProcedure {
  bool DecodeAndExecute(uint8_t *in_bytes, uint32_t *in_len,
                        uint8_t *out_bytes, uint32_t *out_len,
                        bool *ok) override final {
    std::string x_1;
    std::string x_2;
    
    uint32_t temp_len = *in_len;
    uint32_t used_len = 0;

    //string
    if (!Protocol<std::string>::Decode(in_bytes, in_len, ok, x_1) || !*ok) {
      return false;
    }
    temp_len = temp_len - *in_len; // now in_len = typesize, temp_len = the remaining length
    in_bytes = in_bytes + *in_len;
    used_len = *in_len;
  
    //string
    if (!Protocol<std::string>::Decode(in_bytes, &temp_len, ok, x_2) || !*ok) {
      return false;
    }
    used_len = used_len + temp_len; //now temp_len = typesize

    *in_len = used_len;
    
    using FunctionPointerType = void (Svc::*)(std::string, std::string);
    auto p = func_ptr.To<FunctionPointerType>();
    (((Svc *) instance)->*p)(x_1,x_2);
    *out_len = 0;
    return true;
  }
};



// TASK2: Client-side
// class IntResult : public BaseResult {
//   int r;
//  public:
//   bool HandleResponse(uint8_t *in_bytes, uint32_t *in_len, bool *ok) override final {
//     return Protocol<int>::Decode(in_bytes, in_len, ok, r);
//   }
//   int &data() { return r; }
// };


template<typename T>
class Result : public BaseResult {
    T r;
public:
    bool HandleResponse(uint8_t *in_bytes, uint32_t *in_len, bool *ok) override final {
      return Protocol<T>::Decode(in_bytes, in_len, ok, r);
    }
    T &data() { return r; }
};

template<>
class Result<void> : public BaseResult {
  public:
    bool HandleResponse(uint8_t *in_bytes, uint32_t *in_len, bool *ok) final {
      *in_len = 0;
      return true;
    }
};

// TASK2: Client-side
class Client : public BaseClient {
 public:
    template <typename Svc>
    Result<int> *Call(Svc *svc, int (Svc::*func)(int), int x) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      // This incomplete solution only works for this type of member functions.
      // So the result must be an integer.
      auto result = new Result<int>();

      // We also send the paramters of the functions. For this incomplete
      // solution, it must be one integer.
      if (!Send(instance_id, func_id, new IntParam(x), result)) {
        // Fail to send, then delete the result and return nullptr.
        delete result;
        return nullptr;
      }
      return result;
    }

    template<typename Svc>
    Result<void> *Call(Svc *svc, void (Svc::*func)()) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<void>();
      Send(instance_id, func_id, new VoidParam(), result);
      return nullptr;
    }

    //Function (2)
    template <typename Svc>
    Result<bool> *Call(Svc *svc, bool (Svc::*func)()) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<bool>();

      if (!Send(instance_id, func_id, new VoidParam(), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }

    //Function (3)
    template <typename Svc>
    Result<std::string> *Call(Svc *svc, std::string (Svc::*func)(unsigned int), unsigned int x) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<std::string>();

      if (!Send(instance_id, func_id, new UnIntParam(x), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }

    //Function (4)
    template <typename Svc>
    Result<std::string> *Call(Svc *svc, std::string (Svc::*func)(std::string), std::string x) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<std::string>();

      if (!Send(instance_id, func_id, new StrParam(x), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }

    //function (5)
    template <typename Svc>
    Result<std::string> *Call(Svc *svc, std::string (Svc::*func)(std::string,int), std::string x_1, int x_2) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<std::string>();

      if (!Send(instance_id, func_id, new StrIntParam(x_1,x_2), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }

    //Function (6) -1
    template <typename Svc>
    Result<unsigned int> *Call(Svc *svc, unsigned int (Svc::*func)(std::string,int), std::string x_1, int x_2) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<unsigned int>();

      if (!Send(instance_id, func_id, new StrIntParam(x_1,x_2), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }
    
    //Function (6) -2
    template <typename Svc>
    Result<unsigned long> *Call(Svc *svc, unsigned long (Svc::*func)(int,unsigned int), int x_1, unsigned int x_2) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<unsigned long>();

      if (!Send(instance_id, func_id, new IntUnIntParam(x_1,x_2), result)) { 
        delete result;
        return nullptr;
      }
      return result;
    }

    //function (7) - 1
    template <typename Svc>
    Result<void> *Call(Svc *svc, void (Svc::*func)(std::string,int), std::string x_1, int x_2) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<void>();

      if (!Send(instance_id, func_id, new StrIntParam(x_1,x_2), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }

    //function (7) - 2
    template <typename Svc>
    Result<void> *Call(Svc *svc, void (Svc::*func)(std::string,std::string), std::string x_1, std::string x_2) {
      // Lookup instance and function IDs.
      int instance_id = svc->instance_id();
      int func_id = svc->LookupExportFunction(MemberFunctionPtr::From(func));

      auto result = new Result<void>();

      if (!Send(instance_id, func_id, new StrStrParam(x_1,x_2), result)) {
        delete result;
        return nullptr;
      }
      return result;
    }
     

};


// TASK2: Server-side
template<typename Svc>
class Service : public BaseService {
 protected:
  // Sample function
  void Export(int (Svc::*func)(int)) {
    ExportRaw(MemberFunctionPtr::From(func), new Int_IntProcedure<Svc>());
  }

  //Function (1)
  void Export(void (Svc::*func)()) {
    ExportRaw(MemberFunctionPtr::From(func), new VoidProcedure<Svc>());
  }

  //Function (2)
  void Export(bool (Svc::*func)()) {
    ExportRaw(MemberFunctionPtr::From(func), new BoolProcedure<Svc>());
  }

  //Function (3)
  void Export(std::string (Svc::*func)(unsigned int)) {
    ExportRaw(MemberFunctionPtr::From(func), new StringProcedure<Svc>());
  }

  //Function (4)
  void Export(std::string (Svc::*func)(std::string)) {
    ExportRaw(MemberFunctionPtr::From(func), new StrStrProcedure<Svc>());
  }

  //Function (5)
  void Export(std::string (Svc::*func)(std::string, int)) {
    ExportRaw(MemberFunctionPtr::From(func), new StrIntProcedure<Svc>());
  }

  //Function (6) - 1
  void Export(unsigned int (Svc::*func)(std::string, int)) {
    ExportRaw(MemberFunctionPtr::From(func), new UnInt_StrIntProcedure<Svc>());
  }

  //Function (6) - 2
  void Export(unsigned long (Svc::*func)(int, unsigned int)) {
    ExportRaw(MemberFunctionPtr::From(func), new Long_IntUnIntProcedure<Svc>());
  }

  //Function (7) - 1
  void Export(void (Svc::*func)(std::string, int)) {
    ExportRaw(MemberFunctionPtr::From(func), new Void_StrIntProcedure<Svc>());
  }

  //Function (7) - 2
  void Export(void (Svc::*func)(std::string, std::string)) {
    ExportRaw(MemberFunctionPtr::From(func), new Void_StrStrProcedure<Svc>());
  }
  
};

}

#endif /* RPCXX_H */
