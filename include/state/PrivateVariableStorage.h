//
// Created by pill on 19-5-2.
//

#ifndef PDES_MAS_PRIVATEVARIABLESTORAGE_H
#define PDES_MAS_PRIVATEVARIABLESTORAGE_H


#include <map>
#include <AbstractValue.h>
#include <types/SerialisableList.h>
#include "WritePeriod.h"

using namespace std;
namespace pdesmas {
  class PrivateVariableStorage {
  private:
    map<unsigned long, SerialisableList<WritePeriod> *> variable_id_wp_map_ = map<unsigned long, SerialisableList<WritePeriod> *>();

  public:
    bool AddVariable(const unsigned long variable_id);

    AbstractValue *ReadVariable(const unsigned long variable_id, unsigned long timestamp);

    bool WriteVariable(const unsigned long variable_id, AbstractValue *value, unsigned long timestamp);

    void PerformRollback(unsigned long timestamp);

    void CleanUp(unsigned long timestamp);
  };
}


#endif //PDES_MAS_PRIVATEVARIABLESTORAGE_H
