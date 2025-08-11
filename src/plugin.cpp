#include "plugin.hpp"

Plugin* plugin_instance;

void init(Plugin* p) {
    plugin_instance = p;
    p->addModel(model_opc_vcv_ir);
}
