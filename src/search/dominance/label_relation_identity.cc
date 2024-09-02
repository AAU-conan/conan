#include "label_relation_identity.h"

using namespace std;

namespace dominance {
    LabelRelationIdentity::LabelRelationIdentity(Labels *_labels) : labels(_labels),
                                                                    num_labels(_labels->get_size()) {
    }

}