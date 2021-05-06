#include "input.hpp"

#include <cstring>
#include <getopt.h>
#include <fstream>
#include <sstream>

static const char COMMENT_CHARS[] = "#\0//\0--\0%\0";
static const char DELIMITER[] = "\0";
static const char CONFIG_DELIMITER[] = "=";
#define COMMENT_CHARS_SIZE (sizeof(COMMENT_CHARS)/sizeof(char))

static void get_opt_argc_argv( vector<int>& opt_idx, const vector<char*>& argv );
static InputKey** alloc_inkey_array_from_opt( const vector<int>& opt_idx, const vector<char*>& argv );
static void free_inkey_array( InputKey** result, uint size );

static ostream& operator<<( ostream& stream, const vector<double>& v ){
    if (v.size() == 0) return stream;
    for (uint i=0; i<v.size() - 1; ++i)
        stream << v[i] << ",";
    stream << v[v.size()-1];
    return stream;
}

static void print_help( const char* scriptname );
static void check_if_has_long_and_delimiter( int argc, char** argv );
static void parse_short_opts( int* p_argc, char*** p_argv,
        string& conffile, bool& dump, double& value, string& mode, bool& verbose );

class InputKey {
    private:
        char _comment_chars[COMMENT_CHARS_SIZE];
        string* _comment_strings;
        int _num_comment_strings;
        void _check_comment( string );
        void _splitup( string );
    public:
        InputKey( string );
        ~InputKey();
        bool comment;
        string key;
        string val;
};

void InputKey::_check_comment( string line ) {
    comment = false;
    _num_comment_strings = 0;
    _comment_strings = new string[COMMENT_CHARS_SIZE];
    for (uint i=0; i<COMMENT_CHARS_SIZE; ++i) {
        if ( !(COMMENT_CHARS[i] == DELIMITER[0]) ) {
            _comment_strings[_num_comment_strings] += COMMENT_CHARS[i];
        } else {
            ++_num_comment_strings;
        }
    }

    int iscomment = 0;
    for (int i=0; i<_num_comment_strings; ++i) {
        uint subcomment = 0;
        for (uint j=0; j<_comment_strings[i].size(); ++j) {
            if ( _comment_strings[i][j] == line[j] ) {
                ++subcomment;
            }
        }
        if (subcomment == _comment_strings[i].size()) {
            ++iscomment;
        }
    }
    if (iscomment > 1) {
        comment = true;
        key = "\0";
        val = "\0";
    }
}

void InputKey::_splitup( string line ) {
    size_t loc = line.find_first_of(CONFIG_DELIMITER);
    if (loc<=line.size()) {
        key = line.substr(0,loc);
        if (loc == line.size()) val = "";
        else val = line.substr(loc+1,line.size());
    } else {
        std::cerr << "use syntax '" << line << "=val'" << std::endl;
        key = "\0";
        val = "\0";
    }
}

InputKey::InputKey( string line ) {
    _check_comment( line );
    if (!comment) {
        _splitup( line );
    }
}

InputKey::~InputKey() {
    delete[] _comment_strings;
}

void get_opt_argc_argv( vector<int>& opt_idx, const vector<char*>& argv ) {
    opt_idx.resize(0);
    for (uint i=0; i<argv.size(); ++i) {
        char* arg = argv[i];

        bool is_opt = false;
        if (strlen(arg) > 2) {
            if (arg[0] == '-' && arg[1] == '-') is_opt = true;
        }

        if (is_opt) {
            opt_idx.push_back(i);
        } else if (arg[2] != '\0') {
            // skip all empty options
            std::cerr << "unparsed option: '" << arg << "'" << std::endl;
        }
    }
}

InputKey** alloc_inkey_array_from_opt( const vector<int>& opt_idx, const vector<char*>& argv ) {
    InputKey** result = new InputKey* [opt_idx.size()];
    for (uint i=0; i<opt_idx.size(); ++i) {
        string line = argv[opt_idx[i]] + 2;
        result[i] = new InputKey( line );
    }
    return result;
}

void free_inkey_array( InputKey** result, uint size ) {
    for (uint i=0; i<size; ++i) delete result[i];
    delete[] result;
}

void Input::_options_from_argv_vector( vector<char*>& argv ) {
    vector<int> opt_idx;
    get_opt_argc_argv( opt_idx, argv );
    InputKey** inkeys = alloc_inkey_array_from_opt( opt_idx, argv );
    for (uint i=0; i<opt_idx.size(); ++i) {
        if (inkeys[i]->key != "\0") {
            _dump_self += _callback_func( inkeys[i] );
        }
    }
    free_inkey_array( inkeys, opt_idx.size() );
    for (auto i = opt_idx.rbegin(); i != opt_idx.rend(); ++i) {
        argv.erase( argv.begin() + *i );
    }
}

void Input::_options_from_argc_argv( int argc, char** argv ) {
    if (argc<=1) return;

    vector<char*> vec_argv(argc-1);
    for (int i=1; i<argc; ++i) vec_argv[i-1] = argv[i];
    _options_from_argv_vector( vec_argv );

    _argc = vec_argv.size() + 1;
    _argv = (char**) malloc( sizeof(char*) * _argc );

    for (int i=1; i<_argc; ++i) _argv[i] = vec_argv[i-1];
    _argv[0] = argv[0];
}

Input::Input( int* p_argc, char*** p_argv ) {
    bool dump_input_extra = false;
    string conffile = "";
    check_if_has_long_and_delimiter( *p_argc, *p_argv );
    parse_short_opts( p_argc, p_argv, conffile, dump_input_extra, value, mode, verbose );

    if (conffile != "") setConfigFile( conffile );
    _read_config_file();
    _options_from_argc_argv( *p_argc, *p_argv );

    if (mode == "") {
        mode = "auto";
    }
    if (value == -1.0) {
        value = 1.0;
    }

    chosenFunction = chooseFunction( functionChoice );

    if (dump_input_extra || _dump_self) {
        std::cout << *this << std::endl;
        exit(1);
    }
}

Input::~Input() {
    if (_argv != nullptr) {
        //free(_argv);
    }
}

void Input::_read_config_file() {
    std::ifstream infile(_configFile);
    if (!infile) {
        if (_configFile != "")
            std::cerr << "conffile '" << _configFile << "' does not exist. Using defaults." << std::endl;
    } else {
        while (infile) {
            string current_line;
            getline(infile, current_line);
            if (current_line != "\0") {
                InputKey inkey( current_line );
                _dump_self += _callback_func( &inkey );
            }
        }
    }
    infile.close();
}

template<typename T>
static vector<T> split(const string& str) {
    const string delim = ",";
    vector<T> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim,prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (typeid(T) == typeid(int)) {
            if (!token.empty()) tokens.push_back(atoi(token.c_str()));
        } else {
            if (!token.empty()) tokens.push_back(atof(token.c_str()));
        }
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

#define CALLBACK_bool( val ) \
    if ((v == "true") || (v == "True") || (atoi(v.c_str()) == 1)) val = true; \
    else if ((v == "false") || (v == "False") || (atoi(v.c_str()) == 0)) val = false; \
    else std::cerr <<  "'" << k << "' takes [true|True|1|false|False|0]" << std::endl;
#define CALLBACK_int( val )    val = atoi(v.c_str());
#define CALLBACK_double( val ) val = atof(v.c_str());
#define CALLBACK_string( val ) val = v;
#define CALLBACK_c_str( val ) strcpy( val, v.c_str() );
#define CALLBACK_expr( val )   val
#define CALLBACK_VecNT( val, N, T ) \
    vector<T> val_vec = split<T>(v); \
    if (val_vec.size() != N) \
        std::cerr << "'" << k << "' takes '<" #T ">,…' (dim " << N << ")" << std::endl; \
    if (val_vec.size() < N) { \
        for (int i=0; i<N; ++i) val_vec.push_back(T(0.0)); \
    } \
    for (int i=0; i<N; ++i) val[i] = val_vec[i];
#define CALLBACK_Vec2d( val ) \
    CALLBACK_VecNT( val, 2, double )
#define CALLBACK_Vec3d( val ) \
    CALLBACK_VecNT( val, 3, double )
#define CALLBACK_Vec4d( val ) \
    CALLBACK_VecNT( val, 4, double )
#define CALLBACK_stdAry2i( val ) \
    CALLBACK_VecNT( val, 2, int )
#define CALLBACK_stdAry4i( val ) \
    CALLBACK_VecNT( val, 4, int )
#define CALLBACK_stdAry8i( val ) \
    CALLBACK_VecNT( val, 8, int )
#define CALLBACK_stdVecXi( val ) \
    val = split<int>(v);
#define CALLBACK_stdVecXd( val ) \
    val = split<double>(v);

// can take one of [bool, int, double, string, expr]
#define CALLBACK( key, val, dtype ) \
    else if (k == key) { \
        CALLBACK_##dtype( val ) \
    }
#define EASY_CALLBACK( key, dtype ) \
    CALLBACK( #key, key, dtype )

bool Input::_callback_func( InputKey* inkey ) {

    bool dump_self = false;
    string k = inkey->key;
    string v = inkey->val;
    if (inkey->comment) {
        return dump_self;
    }

    EASY_CALLBACK( webcamDevice, string )
    EASY_CALLBACK( webcamWidth, int )
    EASY_CALLBACK( webcamHeight, int )
    EASY_CALLBACK( webcamLightValLow, int )
    EASY_CALLBACK( webcamLightValHigh, int )

    EASY_CALLBACK( backlightDevice, string )
    EASY_CALLBACK( minBrightness, int )
    EASY_CALLBACK( maxBrightness, int )

    EASY_CALLBACK( readMinMaxBrightness, bool )
    EASY_CALLBACK( minAfterRead, double )

    EASY_CALLBACK( keyboardDevice, string )
    EASY_CALLBACK( keyboardMaxBrightness, int )
    EASY_CALLBACK( keyboardMinBrightness, int )
    EASY_CALLBACK( useKeyboard, bool )

    EASY_CALLBACK( functionChoice, string )
    EASY_CALLBACK( functionParams, stdVecXd )

    CALLBACK( "dump_input", _dump_self, bool )

    else {
        std::cerr << "bad config key '" << k << "'" << std::endl;
    }
    return dump_self;
}

static const int _padlength = 30;
template<typename T>
string print_param_str( string param, string help, T val ) {
    std::stringstream result;
    int pad = _padlength - param.length();
    result << param << string(pad,' ') << help << " (default: " << val << ")";
    return result.str();
}

ostream& operator<<( ostream& stream, const Input& inp ) {
#define print_param( param, help ) \
    "\n  " << print_param_str( #param, help, inp.param ) <<
    stream << "long options:" <<
        print_param(webcamDevice,              "sysfs/dev path for webcam device")
        print_param(webcamWidth,               "number of pixels (width)")
        print_param(webcamHeight,              "number of pixels (height)")
        print_param(webcamLightValLow,         "light level value for darkness")
        print_param(webcamLightValHigh,        "light level value for brightness")

        print_param( backlightDevice,          "sysfs path for backlight" )
        print_param( minBrightness,            "minimum value used" )
        print_param( maxBrightness,            "maximum value used" )

        print_param( readMinMaxBrightness,     "read min & max values from sysfs files" )
        print_param( minAfterRead,             "min/max ratio when reading from files" )

        print_param( keyboardDevice,           "sysfs keyboard backlight path" )
        print_param( keyboardMaxBrightness,    "keyboard device maximum value" )
        print_param( keyboardMinBrightness,    "keyboard device minimum value" )
        print_param( useKeyboard,              "use keyboard device" )

        print_param( functionChoice,           "choice of mapping between webcam and backlight")
        print_param( functionParams,           "parameters for mappings" )

        "";
    return stream;
#undef print_param
}

static void print_help( const char* scriptname ) {
    std::cerr  << "usage: '" << scriptname << " [short opts] -- [long opts]'\n"
               << "  with [short opts]\n"
               << "    -c [config file] (default: '" CONFIG_FILE "')\n"
               << "    -m [mode] (set, inc, dec, auto, toggle, info)\n"
               << "    -V [value] (used when mode is 'set', 'inc' or 'dec')\n"
               << "    -v enable verbose logging\n"
               << "    -h print this help\n"
               << "    -d print long parameters\n"
               << "  and [long opts] from -d (format: --<key>=<value>)\n"
               << "caution: [short opts] must preceed [long opts]!\n"
               << "         The double dash between [short opts] and\n"
               << "         [long opts] (--) is mandatory!\n";
    exit(1);
}

static void check_if_has_long_and_delimiter( int argc, char** argv ) {
    bool has_delim = false;
    bool has_long = false;
    for (int i=0; i<argc; ++i) {
        char* c_argv = argv[i];
        int len = strlen(c_argv);
        if (len == 2) {
            has_delim = (c_argv[0] == '-' && c_argv[1] == '-');
        } else if (len > 2) {
            has_long = (c_argv[0] == '-' && c_argv[1] == '-');
        }
    }
    if (has_long && !has_delim) {
        std::cerr << "missing double dash delimiter. exiting." << std::endl;
        exit (1);
    }
}

static void parse_short_opts( int* p_argc, char*** p_argv, string& conffile,
        bool& dump, double& value, string& mode, bool& verbose ) {
    opterr = 0;
    int c;
    while ((c = getopt( *p_argc, *p_argv, "c:dhm:vV:" )) != -1) {
        switch (c) {
            case 'c':
                conffile = optarg;
                break;
            case 'h':
                print_help( (*p_argv)[0] );
                break;
            case 'd':
                dump = true;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'V':
                value = atof(optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case '?':
                std::cerr << "unknown option '-" << string(1,optopt) << "'" << std::endl;
                break;
            default:
                break;
        }
    }
    *p_argv += optind-1;
    *p_argc -= optind-1;
}
