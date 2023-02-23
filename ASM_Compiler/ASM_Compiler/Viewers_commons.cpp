#include <iostream>
#include <regex>
#include <string>
#include <fstream>
#include <vector>
#include <tuple>

#include "bnflite.h"

using namespace std;

using namespace bnf;

#include "Tokenizer.h"
#include "Viewer_commons.h"

int active_seg = 0;

segtab segment_tab;
useridstab user_identifiers_tab;
macrolib macro_library;
macropartab macro_parameters_tab;
macroargtab macro_arguments_tab;
