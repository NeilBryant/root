// @(#)root/pyroot:$Id$
// Author: Wim Lavrijsen, May 2004

// Bindings
#include "PyROOT.h"
#include "TPyClassGenerator.h"
#include "Utility.h"
#include "TPyReturn.h"

// ROOT
#include "TClass.h"
#include "TInterpreter.h"

// Standard
#include <sstream>
#include <string>
#include <typeinfo>


//- public members -----------------------------------------------------------
TClass* TPyClassGenerator::GetClass( const char* name, Bool_t load )
{
// Just forward.
   return GetClass( name, load, kFALSE );
}

//- public members -----------------------------------------------------------
TClass* TPyClassGenerator::GetClass( const char* name, Bool_t load, Bool_t silent )
{
// Class generator to make python classes available to Cling

// called if all other class generators failed, attempt to build from python class
   if ( PyROOT::gDictLookupActive == kTRUE )
      return 0;                              // call originated from python

   if ( ! load || ! name )
      return 0;

// first, check whether the name is of a module
   PyObject* modules = PySys_GetObject( const_cast<char*>("modules") );
   PyObject* pyname = PyROOT_PyUnicode_FromString( name );
   PyObject* keys = PyDict_Keys( modules );
   Bool_t isModule = PySequence_Contains( keys, pyname );
   Py_DECREF( keys );
   Py_DECREF( pyname );

   if ( isModule ) {
      std::ostringstream nsCode;
      nsCode << "namespace " << name << " { }";

      if ( gInterpreter->LoadText( nsCode.str().c_str() ) ) {
          TClass* klass = new TClass( name, silent );
          TClass::AddClass( klass );
          return klass;
      }

      return nullptr;
   }

// determine module and class name part
   std::string clName = name;
   std::string::size_type pos = clName.rfind( '.' );

   if ( pos == std::string::npos )
      return 0;                              // this isn't a python style class

   std::string mdName = clName.substr( 0, pos );
   clName = clName.substr( pos+1, std::string::npos );

// create class in in namespace, if it exists (no load, silent)
   Bool_t useNS = TClass::GetClass( mdName.c_str(), kFALSE, kTRUE ) != 0;

   if ( ! useNS ) {
   // the class itself may exist if we're using the global scope
      TClass* klass = TClass::GetClass( clName.c_str(), kFALSE, kTRUE );
      if ( klass ) return klass;
   }

// locate and get class
   PyObject* mod = PyImport_AddModule( const_cast< char* >( mdName.c_str() ) );
   if ( ! mod ) {
      PyErr_Clear();
      return 0;                              // module apparently disappeared
   }

   Py_INCREF( mod );
   PyObject* pyclass =
      PyDict_GetItemString( PyModule_GetDict( mod ), const_cast< char* >( clName.c_str() ) );
   Py_XINCREF( pyclass );
   Py_DECREF( mod );

   if ( ! pyclass ) {
      PyErr_Clear();                         // the class is no longer available?!
      return 0;
   }

// get a listing of all python-side members
   PyObject* attrs = PyObject_Dir( pyclass );
   if ( ! attrs ) {
      PyErr_Clear();
      Py_DECREF( pyclass );
      return 0;
   }

// pre-amble Cling proxy class
   std::ostringstream proxyCode;
   if ( useNS ) proxyCode << "namespace " << mdName << " { ";
   proxyCode << "class " << clName << " {\nprivate:\n PyObject* fPyObject;\npublic:\n";

// loop over and add member functions
   Bool_t hasConstructor = kFALSE;
   for ( int i = 0; i < PyList_GET_SIZE( attrs ); ++i ) {
      PyObject* label = PyList_GET_ITEM( attrs, i );
      Py_INCREF( label );
      PyObject* attr = PyObject_GetAttr( pyclass, label );

   // collect only member functions (i.e. callable elements in __dict__)
      if ( PyCallable_Check( attr ) ) {
         std::string mtName = PyROOT_PyUnicode_AsString( label );

      // figure out number of variables required
         PyObject* im_func = PyObject_GetAttrString( attr, (char*)"im_func" );
         PyObject* func_code =
            im_func ? PyObject_GetAttrString( im_func, (char*)"func_code" ) : NULL;
         PyObject* var_names =
            func_code ? PyObject_GetAttrString( func_code, (char*)"co_varnames" ) : NULL;
         int nVars = var_names ? PyTuple_GET_SIZE( var_names ) - 1 /* self */ : 0 /* TODO: probably large number, all default? */;
         if ( nVars < 0 ) nVars = 0;
         Py_XDECREF( var_names );
         Py_XDECREF( func_code );
         Py_XDECREF( im_func );

         Bool_t isConstructor = mtName == "__init__";
         Bool_t isDestructor  = mtName == "__del__";

      // method declaration as appropriate
         if ( isConstructor ) {
            hasConstructor = kTRUE;
            proxyCode << " " << clName << "(";
         } else if ( isDestructor )
            proxyCode << " ~" << clName << "(";
         else // normal method
            proxyCode << " TPyReturn " << mtName << "(";
         for ( int ivar = 0; ivar < nVars; ++ivar ) {
             proxyCode << "const TPyArg& a" << ivar;
             if ( ivar != nVars-1 ) proxyCode << ", ";
         }
         proxyCode << ") {\n";
         proxyCode << "  std::vector<TPyArg> v; v.reserve(" << nVars+(isConstructor ? 0 : 1) << ");\n";

      // add the 'self' argument as appropriate
         if ( ! isConstructor )
            proxyCode << "  v.push_back(fPyObject);\n";

      // then add the remaining variables
         for ( int ivar = 0; ivar < nVars; ++ ivar )
            proxyCode << "  v.push_back(a" << ivar << ");\n";

      // call dispatch (method or class pointer hard-wired)
         if ( ! isConstructor )
            proxyCode << "  return TPyReturn(TPyArg::CallMethod((PyObject*)" << (void*)attr << ", v))";
         else
            proxyCode << "  TPyArg::CallConstructor(fPyObject, (PyObject*)" << (void*)pyclass << ", v)";
         proxyCode << ";\n }\n";
      }

      // no decref of attr for now (b/c of hard-wired ptr); need cleanup somehow
      Py_DECREF( label );
   }

// special case if no constructor
   if ( ! hasConstructor )
      proxyCode << " " << clName << "() {\n TPyArg::CallConstructor(fPyObject, (PyObject*)" << (void*)pyclass << "); }\n";

// closing and building of Cling proxy class
   proxyCode << "};";
   if ( useNS ) proxyCode << " }";

   Py_DECREF( attrs );
// done with pyclass, decref here, assuming module is kept
   Py_DECREF( pyclass );

// body compilation
   if ( ! gInterpreter->LoadText( proxyCode.str().c_str() ) )
      return nullptr;

// done, let ROOT manage the new class
   TClass* klass = new TClass( useNS ? (mdName+"::"+clName).c_str() : clName.c_str(), silent );
   TClass::AddClass( klass );

   return klass;
}

//____________________________________________________________________________
TClass* TPyClassGenerator::GetClass( const type_info& typeinfo, Bool_t load, Bool_t silent )
{
// Just forward; based on type name only.
   return GetClass( typeinfo.name(), load, silent );
}

//____________________________________________________________________________
TClass* TPyClassGenerator::GetClass( const type_info& typeinfo, Bool_t load )
{
// Just forward; based on type name only
   return GetClass( typeinfo.name(), load );
}
