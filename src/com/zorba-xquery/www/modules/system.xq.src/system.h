/*
 * Copyright 2006-2008 The FLWOR Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __COM_ZORBA_WWW_MODULES_SYSTEM_H__
#define __COM_ZORBA_WWW_MODULES_SYSTEM_H__
#include <vector>
#include <map>

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/function.h>

namespace zorba { namespace system {
  class SystemModule : public ExternalModule {
    private:
      ExternalFunction* thePropertyFunction;
      ExternalFunction* thePropertiesFunction;
      const static String SYSTEM_MODULE_NAMESPACE;
    public:
      SystemModule();
      virtual ~SystemModule();
    public:
      virtual String getURI() const { return SYSTEM_MODULE_NAMESPACE; }

      virtual ExternalFunction* getExternalFunction(const String& localName);

      virtual void destroy();
  };

  class SystemFunction {
    protected:
      const ExternalModule* theModule;
      ItemFactory* theFactory;
      std::map<String, String> theProperties;
    public:
      SystemFunction(const ExternalModule* aModule);
    protected:
      String getURI() const { return theModule->getURI(); }
      bool getEnv(const String& name, String& value) const;
      void getEnvNames(std::vector<Item>& names) const;
      String intToString(int v);
  };

  class PropertiesFunction : public NonContextualExternalFunction, public SystemFunction {
    public:
      PropertiesFunction(const ExternalModule* mod) : SystemFunction(mod) {}

      virtual String getLocalName() const { return "properties"; }

      virtual ItemSequence_t 
      evaluate(const ExternalFunction::Arguments_t& args) const;
      virtual String getURI() const { return SystemFunction::getURI(); }
  };

  class PropertyFunction : public ContextualExternalFunction, public SystemFunction {
    public:
      PropertyFunction(const ExternalModule* mod) : SystemFunction(mod) {}

      virtual String getLocalName() const { return "property"; }

      virtual ItemSequence_t 
      evaluate(const ExternalFunction::Arguments_t& args,
               const StaticContext* sctx,
               const DynamicContext* dctx) const;
      virtual String getURI() const { return SystemFunction::getURI(); }
  };

} } // namespace zorba, namespace system

#ifdef WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::system::SystemModule();
}

#endif // __COM_ZORBA_WWW_MODULES_SYSTEM_H__
