#pragma once


class NativeScript
{
public:
   NativeScript() {}
   virtual ~NativeScript() {}


   virtual void Start() {};
   virtual void Update() {};
};

typedef NativeScript* create_t();
typedef void destroy_t(NativeScript*);