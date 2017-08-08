#include "pyc_numeric.h"
#include "bytecode.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define DECLARE_PYTHON(maj, min) \
    extern int python_##maj##min##_map(int); \
    extern int python_##maj##min##_unmap(int);

DECLARE_PYTHON(1, 0)
DECLARE_PYTHON(1, 1)
DECLARE_PYTHON(1, 3)
DECLARE_PYTHON(1, 4)
DECLARE_PYTHON(1, 5)
DECLARE_PYTHON(1, 6)
DECLARE_PYTHON(2, 0)
DECLARE_PYTHON(2, 1)
DECLARE_PYTHON(2, 2)
DECLARE_PYTHON(2, 3)
DECLARE_PYTHON(2, 4)
DECLARE_PYTHON(2, 5)
DECLARE_PYTHON(2, 6)
DECLARE_PYTHON(2, 7)
DECLARE_PYTHON(3, 0)
DECLARE_PYTHON(3, 1)
DECLARE_PYTHON(3, 2)
DECLARE_PYTHON(3, 3)
DECLARE_PYTHON(3, 4)
DECLARE_PYTHON(3, 5)
DECLARE_PYTHON(3, 6)

const char* Pyc::OpcodeName(int opcode)
{
    static const char* opcode_names[] = {
        #define OPCODE(x) #x,
        #define OPCODE_A_FIRST(x) #x,
        #define OPCODE_A(x) #x,
        #include "bytecode_ops.inl"
        #undef OPCODE_A
        #undef OPCODE_A_FIRST
        #undef OPCODE
    };

#if __cplusplus >= 201103L
    static_assert(sizeof(opcode_names) / sizeof(opcode_names[0]) == PYC_LAST_OPCODE,
                  "Pyc::OpcodeName opcode_names not in sync with opcode enum");
#endif

    if (opcode < 0)
        return "<INVALID>";

    if (opcode < PYC_LAST_OPCODE)
        return opcode_names[opcode];

    static char badcode[10];
    snprintf(badcode, 10, "<%d>", opcode);
    return badcode;
};

int Pyc::ByteToOpcode(int maj, int min, int opcode)
{
    switch (maj) {
    case 1:
        switch (min) {
        case 0: return python_10_map(opcode);
        case 1: return python_11_map(opcode);
        case 3: return python_13_map(opcode);
        case 4: return python_14_map(opcode);
        case 5: return python_15_map(opcode);
        case 6: return python_16_map(opcode);
        }
        break;
    case 2:
        switch (min) {
        case 0: return python_20_map(opcode);
        case 1: return python_21_map(opcode);
        case 2: return python_22_map(opcode);
        case 3: return python_23_map(opcode);
        case 4: return python_24_map(opcode);
        case 5: return python_25_map(opcode);
        case 6: return python_26_map(opcode);
        case 7: return python_27_map(opcode);
        }
        break;
    case 3:
        switch (min) {
        case 0: return python_30_map(opcode);
        case 1: return python_31_map(opcode);
        case 2: return python_32_map(opcode);
        case 3: return python_33_map(opcode);
        case 4: return python_34_map(opcode);
        case 5: return python_35_map(opcode);
        case 6: return python_36_map(opcode);
        }
        break;
    }
    return PYC_INVALID_OPCODE;
}

bool Pyc::IsConstArg(int opcode)
{
    return (opcode == Pyc::LOAD_CONST_A) || (opcode == Pyc::RESERVE_FAST_A);
}

bool Pyc::IsNameArg(int opcode)
{
    return (opcode == Pyc::DELETE_ATTR_A) || (opcode == Pyc::DELETE_GLOBAL_A) ||
           (opcode == Pyc::DELETE_NAME_A) || (opcode == Pyc::IMPORT_FROM_A) ||
           (opcode == Pyc::IMPORT_NAME_A) || (opcode == Pyc::LOAD_ATTR_A) ||
           (opcode == Pyc::LOAD_GLOBAL_A) || (opcode == Pyc::LOAD_LOCAL_A) ||
           (opcode == Pyc::LOAD_NAME_A) || (opcode == Pyc::STORE_ATTR_A) ||
           (opcode == Pyc::STORE_GLOBAL_A) || (opcode == Pyc::STORE_NAME_A);
}

bool Pyc::IsVarNameArg(int opcode)
{
    return (opcode == Pyc::DELETE_FAST_A) || (opcode == Pyc::LOAD_FAST_A) ||
           (opcode == Pyc::STORE_FAST_A);
}

bool Pyc::IsCellArg(int opcode)
{
    return (opcode == Pyc::LOAD_CLOSURE_A) || (opcode == Pyc::LOAD_DEREF_A) ||
           (opcode == Pyc::STORE_DEREF_A);
}

bool Pyc::IsJumpOffsetArg(int opcode)
{
    return (opcode == Pyc::JUMP_FORWARD_A) || (opcode == Pyc::JUMP_IF_FALSE_A) ||
           (opcode == Pyc::JUMP_IF_TRUE_A) || (opcode == Pyc::SETUP_LOOP_A) ||
           (opcode == Pyc::SETUP_FINALLY_A) || (opcode == Pyc::SETUP_EXCEPT_A) ||
           (opcode == Pyc::FOR_LOOP_A) || (opcode == Pyc::FOR_ITER_A);
}

bool Pyc::IsCompareArg(int opcode)
{
    return (opcode == Pyc::COMPARE_OP_A);
}

void print_const(PycRef<PycObject> obj, PycModule* mod)
{
    if (obj == NULL) {
        fprintf(pyc_output, "<NULL>");
        return;
    }

    switch (obj->type()) {
    case PycObject::TYPE_STRING:
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 'b' : 0);
        break;
    case PycObject::TYPE_UNICODE:
        OutputString(obj.cast<PycString>(), (mod->majorVer() == 3) ? 0 : 'u');
        break;
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        OutputString(obj.cast<PycString>(), 0);
        break;
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        {
            fprintf(pyc_output, "(");
            PycTuple::value_t values = obj.cast<PycTuple>()->values();
            PycTuple::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    fprintf(pyc_output, ", ");
                    print_const(*it, mod);
                }
            }
            if (values.size() == 1)
                fprintf(pyc_output, ",)");
            else
                fprintf(pyc_output, ")");
        }
        break;
    case PycObject::TYPE_LIST:
        {
            fprintf(pyc_output, "[");
            PycList::value_t values = obj.cast<PycList>()->values();
            PycList::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    fprintf(pyc_output, ", ");
                    print_const(*it, mod);
                }
            }
            fprintf(pyc_output, "]");
        }
        break;
    case PycObject::TYPE_DICT:
        {
            fprintf(pyc_output, "{");
            PycDict::key_t keys = obj.cast<PycDict>()->keys();
            PycDict::value_t values = obj.cast<PycDict>()->values();
            PycDict::key_t::const_iterator ki = keys.begin();
            PycDict::value_t::const_iterator vi = values.begin();
            if (ki != keys.end()) {
                print_const(*ki, mod);
                fprintf(pyc_output, ": ");
                print_const(*vi, mod);
                while (++ki != keys.end()) {
                    ++vi;
                    fprintf(pyc_output, ", ");
                    print_const(*ki, mod);
                    fprintf(pyc_output, ": ");
                    print_const(*vi, mod);
                }
            }
            fprintf(pyc_output, "}");
        }
        break;
    case PycObject::TYPE_SET:
        {
            fprintf(pyc_output, "{");
            PycSet::value_t values = obj.cast<PycSet>()->values();
            PycSet::value_t::const_iterator it = values.begin();
            if (it != values.end()) {
                print_const(*it, mod);
                while (++it != values.end()) {
                    fprintf(pyc_output, ", ");
                    print_const(*it, mod);
                }
            }
            fprintf(pyc_output, "}");
        }
        break;
    case PycObject::TYPE_NONE:
        fprintf(pyc_output, "None");
        break;
    case PycObject::TYPE_TRUE:
        fprintf(pyc_output, "True");
        break;
    case PycObject::TYPE_FALSE:
        fprintf(pyc_output, "False");
        break;
    case PycObject::TYPE_ELLIPSIS:
        fprintf(pyc_output, "...");
        break;
    case PycObject::TYPE_INT:
        fprintf(pyc_output, "%d", obj.cast<PycInt>()->value());
        break;
    case PycObject::TYPE_LONG:
        fprintf(pyc_output, "%s", obj.cast<PycLong>()->repr().c_str());
        break;
    case PycObject::TYPE_FLOAT:
        fprintf(pyc_output, "%s", obj.cast<PycFloat>()->value());
        break;
    case PycObject::TYPE_COMPLEX:
        fprintf(pyc_output, "(%s+%sj)", obj.cast<PycComplex>()->value(),
                                        obj.cast<PycComplex>()->imag());
        break;
    case PycObject::TYPE_BINARY_FLOAT:
        fprintf(pyc_output, "%g", obj.cast<PycCFloat>()->value());
        break;
    case PycObject::TYPE_BINARY_COMPLEX:
        fprintf(pyc_output, "(%g+%gj)", obj.cast<PycCComplex>()->value(),
                                        obj.cast<PycCComplex>()->imag());
        break;
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        fprintf(pyc_output, "<CODE> %s", obj.cast<PycCode>()->name()->value());
        break;
    }
}

void bc_next(PycBuffer& source, PycModule* mod, int& opcode, int& operand, int& pos)
{
    opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
    bool py36_opcode = (mod->majorVer() > 3 || (mod->majorVer() == 3 && mod->minorVer() >= 6));
    if (py36_opcode) {
        operand = source.getByte();
        pos += 2;
    } else {
        operand = 0;
        pos += 1;
    }

    if (opcode == Pyc::EXTENDED_ARG_A) {
        if (py36_opcode) {
            opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
            operand <<= 8;
            operand |= source.getByte();
            pos += 2;
        } else {
            operand = source.get16() << 16;
            opcode = Pyc::ByteToOpcode(mod->majorVer(), mod->minorVer(), source.getByte());
            pos += 3;
        }
    }
    if (!py36_opcode && (opcode >= Pyc::PYC_HAVE_ARG)) {
        operand |= source.get16();
        pos += 2;
    }
}

void bc_disasm(PycRef<PycCode> code, PycModule* mod, int indent)
{
    static const char *cmp_strings[] = {
        "<", "<=", "==", "!=", ">", ">=", "in", "not in", "is", "is not",
        "<EXCEPTION MATCH>", "<BAD>"
    };
    static const size_t cmp_strings_len = sizeof(cmp_strings) / sizeof(cmp_strings[0]);

    PycBuffer source(code->code()->value(), code->code()->length());

    int opcode, operand;
    int pos = 0;
    while (!source.atEof()) {
        for (int i=0; i<indent; i++)
            fprintf(pyc_output, "    ");
        fprintf(pyc_output, "%-7d ", pos);   // Current bytecode position

        bc_next(source, mod, opcode, operand, pos);
        fprintf(pyc_output, "%-24s", Pyc::OpcodeName(opcode));

        if (opcode >= Pyc::PYC_HAVE_ARG) {
            if (Pyc::IsConstArg(opcode)) {
                fprintf(pyc_output, "%d: ", operand);
                print_const(code->getConst(operand), mod);
            } else if (Pyc::IsNameArg(opcode)) {
                fprintf(pyc_output, "%d: %s", operand, code->getName(operand)->value());
            } else if (Pyc::IsVarNameArg(opcode)) {
                fprintf(pyc_output, "%d: %s", operand, code->getVarName(operand)->value());
            } else if (Pyc::IsCellArg(opcode)) {
                fprintf(pyc_output, "%d: %s", operand, code->getCellVar(operand)->value());
            } else if (Pyc::IsJumpOffsetArg(opcode)) {
                fprintf(pyc_output, "%d (to %d)", operand, pos+operand);
            } else if (Pyc::IsCompareArg(opcode)) {
                if (static_cast<size_t>(operand) < cmp_strings_len)
                    fprintf(pyc_output, "%d (%s)", operand, cmp_strings[operand]);
                else
                    fprintf(pyc_output, "%d (UNKNOWN)", operand);
            } else {
                fprintf(pyc_output, "%d", operand);
            }
        }
        fprintf(pyc_output, "\n");
    }
}
