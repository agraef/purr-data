/* 
rpn - expression evaluator

Copyright (c)2006-2007 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 2411 $
$LastChangedDate: 2007-12-11 10:44:38 -0500 (Tue, 11 Dec 2007) $
$LastChangedBy: thomas $
*/

#include "rpn.h"
#include "rpn_command.h"
#include "rpn_opcode.h"

#include "rpn_function.h"
#include "rpn_stack.h"
#include "rpn_memory.h"
#include "rpn_cmds.h"

#include "rpn_op_meta.h"
#include "rpn_op_input.h"
#include "rpn_op_stack.h"
#include "rpn_op_vec.h"
#include "rpn_op_mem.h"
#include "rpn_op_debug.h"


#define RPN_VERSION "0.1.4"


class rpn
    : public flext_base
{
    FLEXT_HEADER_S(rpn,flext_base,Setup)

public:
    rpn(int argc,const t_atom *argv)
        : dblprec(false),trig(false),keep(false)
        , memory(NULL),sharename(NULL)
        , stacknr(0),stack(NULL)
    {
        int in,out;
        if(!argc)
            in = 1,out = 1;
        else if(CanbeInt(*argv)) {
            in = GetAInt(*argv),argv++,argc--;
            if(!argc)
                out = 1;
            else if(CanbeInt(*argv))
                out = GetAInt(*argv);
            else
                out = -1;
        }
        else
            in = -1;

        if(in < 0 || out < 0) {
            post("Syntax: %s [in] [out] [@@ formula]",thisName());
            InitProblem();
        }
        else {
            AddInAnything(in?in:1);
            AddOutAnything(out);

            inputs.resize(in);
        }

        ms_stack(0);
    }

    ~rpn()
    {
        if(memory)
            if(sharename)
                pool.Free(sharename);
            else
                delete memory;
    }

    virtual bool Finalize()
    {
        if(flext_base::Finalize()) {
            if(sharename) {
                t_atom at; SetSymbol(at,sharename);
                ms_share(1,&at);
            }
            else
                ms_share(0,NULL);
            return true;
        }
        else
            return false;
    }

    //! Evaluate command list
    void m_bang()
    {
        Eval(commands);
    }

    //! Immediate command list
    void m_cmd(int argc,const t_atom *argv)
    {
        Commands cmds;
        Compile(cmds,argc,argv);
        Eval(cmds);
    }

    //! Push operand
    void m_push(int argc,const t_atom *argv)
    {
        stack->Push(Operand(argc,argv));
    }

    void ms_def(const AtomList &l)
    {
        if(l.Count() && IsSymbol(l[0])) {
            Symbol s = GetSymbol(l[0]);
            Commands *cmds = GetFunction(s);
            Compile(*cmds,l.Count()-1,l.Atoms()+1);
        }
        else
            post("%s - Syntax: @def symbol ...",thisName());
    }

    void ms_commands(const AtomList &l) { Compile(commands,l.Count(),l.Atoms()); }

    void ms_share(int argc,const t_atom *argv)
    {
        Symbol s = argc?GetASymbol(*argv):NULL;
        if(s != sharename || !memory) {

            if(memory) 
                if(sharename) 
                    pool.Free(sharename);
                else
                    delete memory;

            sharename = s;

            if(sharename)
                memory = pool.New(sharename);
            else
                memory = new Memory;
        }
    }

    void ms_share(const AtomList &l) { ms_share(l.Count(),l.Atoms()); }

    void mg_share(AtomList &l) { if(sharename) { l(1); SetSymbol(l[0],sharename); } }

    void m_clear() { FLEXT_ASSERT(memory); memory->clear(); }

    void mg_memory(int ix) 
    { 
        FLEXT_ASSERT(memory);
        try {
            Output(memory->Get(ix),sym_memory,ix);
        }
        catch(Error::MemNotSet) {
            post("%s %s - memory location not set",thisName(),GetString(thisTag()));
        }
    }

    void ms_memory(int argc,const t_atom *argv) 
    { 
        FLEXT_ASSERT(memory);
        if(argc < 2 || !CanbeInt(*argv))
            post("%s %s - setting memory needs index and value",thisName(),GetString(thisTag()));
        else {
            int ix = GetAInt(*argv++); --argc;
            Operand o;
            if(argc == 1)
                o = GetAFloat(*argv);
            else
                o(argc,argv);
            memory->Set(ix,o);
        }
    }

    void ms_stack(int st)
    {
        stacknr = st;
        Stacks::iterator it = stacks.find(stacknr);
        if(it == stacks.end())
            stacks[stacknr] = stack = new Stack;
        else
            stack = it->second;
    }

protected:
    typedef std::map<int,Stack *> Stacks;

    int stacknr;
    Stacks stacks;
    Stack *stack;
    Inputs inputs;
    Memory *memory;

    Commands commands;
    bool dblprec,trig,keep;
    Symbol sharename;
    Functions local;
    static Registry<Opcode> opcodes;
    static Registry<Meta> metas;
    static Functions global;
    static MemoryPool pool;

    static Symbol sym_double,sym_set,sym_memory;
    static Symbol sym_popen,sym_pclose;

    Commands *GetFunction(Symbol s)
    {
        Functions &funs = strchr(GetString(s),'.')?global:local;
        return funs.Find(s);
    }

    void Output(int ix,const Operand &result)
    {
        if(result.Scalar()) {
            if(dblprec)
                ToOutDouble(ix,result.to_double());
            else 
                ToOutFloat(ix,result.to_float());
        }
        else {
            AtomListStatic<8> lst;
            result.to_list(lst);
            ToOutList(ix,lst);
        }
    }

    void Output(const Operand &result,Symbol tag,int index)
    {
        if(result.Scalar()) {
            t_atom at[2]; 
            SetInt(at[0],index);
            SetFloat(at[1],result.to_float());
            ToOutAnything(GetOutAttr(),tag,2,at);
        }
        else {
            AtomListStatic<8> lst;
            result.to_list(lst,1);
            SetInt(lst[0],index);
            ToOutAnything(GetOutAttr(),tag,lst);
        }
    }

    void Compile(Commands &commands,int argc,const t_atom *argv)
    {
        commands.clear();
        Meta *meta = NULL;
        for(int i = 0; i < argc; ++i) {
            Command *cmd = NULL;
            if(CanbeFloat(argv[i])) {
                // constant
                Operand op = GetAFloat(argv[i]);
                cmd = new Constant(op);
            }
            else if(IsSymbol(argv[i])) {
                Symbol s = GetSymbol(argv[i]);
                if(s == sym_popen || s == sym_pclose) {
                    // ignore
                }
                else {
                    const char *t = GetString(s);
                    if(t[0] == '_' && t[1] >= '0' && t[1] <= '9') {
                        // variable
                        int var = atoi(t+1);
                        cmd = new Variable(var);
                    }
                    else {
                        // operation
                        // search for built-in
                        if((cmd = opcodes.Find(s)) != NULL) {
                            if(meta) {
                                cmd = new MetaCmd(meta,cmd);
                                meta = NULL;
                            }
                        }
                        else if((meta = metas.Find(s)) != NULL) {
                        }
                        else
                            // find function
                            cmd = new Function(GetFunction(s));
                    }
                }
            }
            else
                throw Error::OpNotDef();

            if(cmd) commands.push_back(cmd);
        }
    }

    void Eval(Commands &c)
    {
        FLEXT_ASSERT(stack);

        if(!keep)
            stack->clear();

        try {
            c.Do(*stack,inputs,*memory);

            if(stack->empty())
                ToOutBang(0);
            else {
                int out = CntOut();
                int sz = (int)stack->size();
                int cnt = out < sz?out:sz;
                Stack::reverse_iterator it = stack->rbegin(); it += cnt-1;
                for(int i = 0; i < cnt; ++i) {
                    Output(cnt-1-i,*it);
                    it--;
                }
            }
        }
        catch(Error::OpTypeErr) {
            post("%s - type mismatch",thisName());
        }
        catch(Error::OpNotImp) {
            post("%s - operation not implemented",thisName());
        }
        catch(Error::OpUnderflow) {
            post("%s - stack underflow",thisName());
        }
        catch(Error::OpBounds) {
            post("%s - vector bounds exceeded",thisName());
        }
        catch(Error::MemNotSet) {
            post("%s - memory location not set",thisName());
        }
    }

    FLEXT_CALLBACK(m_bang)
    FLEXT_CALLBACK_V(m_cmd)
    FLEXT_CALLBACK_V(m_push)
    FLEXT_CALLBACK(m_clear)
    FLEXT_CALLBACK_I(mg_memory)
    FLEXT_CALLBACK_V(ms_memory)
    FLEXT_ATTRGET_I(stacknr)
    FLEXT_CALLSET_I(ms_stack)
    FLEXT_CALLSET_V(ms_commands)
    FLEXT_CALLSET_V(ms_def)
    FLEXT_ATTRVAR_B(dblprec)
    FLEXT_ATTRVAR_B(trig)
    FLEXT_ATTRVAR_B(keep)
    FLEXT_CALLVAR_V(mg_share,ms_share)

    virtual bool CbMethodResort(int inlet,Symbol s,int argc,const t_atom *argv)
    {
        Operand &op = inputs[inlet];
        if(s == sym_float) {
            FLEXT_ASSERT(argc == 1 && IsFloat(*argv));
            op = GetFloat(*argv);
        }
        else if(s == sym_list || s == sym_set)
            op(argc,argv);
        else if(s == sym_double && argc == 2 && CanbeFloat(argv[0]) && CanbeFloat(argv[1]))
            op = (double)GetAFloat(argv[0])+(double)GetAFloat(argv[1]);
        else
            return false;

        if(s != sym_set && (inlet == 0 || trig)) 
            m_bang();

        return true;
    }

    static void Setup(t_classid c)
    {
        sym_double = MakeSymbol("double");
        sym_set = MakeSymbol("set");
        sym_memory = MakeSymbol("?");

	    post("-------------------------------");
	    post("rpn - expression evaluator");
        post("version " RPN_VERSION " (c)2006-2007 Thomas Grill");
    #ifdef FLEXT_DEBUG
        post("");
        post("DEBUG BUILD - " __DATE__ " " __TIME__);
    #endif
	    post("-------------------------------");

        FLEXT_CADDMETHOD(c,0,m_bang);
        FLEXT_CADDMETHOD_(c,0,"cmd",m_cmd);
        FLEXT_CADDMETHOD_(c,0,"push",m_push);
        FLEXT_CADDMETHOD_(c,0,"clear",m_clear);
        FLEXT_CADDMETHOD_(c,0,"get?",mg_memory);
        FLEXT_CADDMETHOD_(c,0,"?",ms_memory);

        FLEXT_CADDATTR_SET(c,"@",ms_commands);
        FLEXT_CADDATTR_SET(c,"def",ms_def);
        FLEXT_CADDATTR_VAR1(c,"dblprec",dblprec);
        FLEXT_CADDATTR_VAR1(c,"trig",trig);
        FLEXT_CADDATTR_VAR1(c,"keep",keep);
        FLEXT_CADDATTR_VAR(c,"share",mg_share,ms_share);
        FLEXT_CADDATTR_VAR(c,"stack",stacknr,ms_stack);

        // register opcodes /////////////////////////

        // ignored
        sym_popen = MakeSymbol("(");
        sym_pclose = MakeSymbol(")");

        // meta command
        metas.Register("do",new OpMeta::MetaDo);
        metas.Register("while",new OpMeta::MetaWhile);

        // no args
        opcodes.Register("pi",new ImpNonary<Basic::Pi>);
        opcodes.Register("rand",new ImpNonary<Basic::Random>);
        opcodes.Register("time",new ImpNonary<Basic::Time>);

        // unary
        opcodes.Register("neg",new ImpUnary<Basic::Neg>);
        opcodes.Register("inv",new ImpUnary<Basic::Inv>);
        opcodes.Register("++",new ImpUnary<Basic::Inc>);
        opcodes.Register("--",new ImpUnary<Basic::Dec>);

        opcodes.Register("int",new ImpUnary<Basic::Int>);
        opcodes.Register("ceil",new ImpUnary<Basic::Ceil>);
        opcodes.Register("floor",new ImpUnary<Basic::Floor>);
        opcodes.Register("round",new ImpUnary<Basic::Round>);

        opcodes.Register("sqr",new ImpUnary<Basic::Sqr>);
        opcodes.Register("sqrt",new ImpUnary<Basic::Sqrt>);

        opcodes.Register("exp",new ImpUnary<Basic::Exp>);
        opcodes.Register("ln",new ImpUnary<Basic::Ln>);
        opcodes.Register("lg",new ImpUnary<Basic::Lg>);
        opcodes.Register("ld",new ImpUnary<Basic::Ld>);

        opcodes.Register("sin",new ImpUnary<Basic::Sin>);
        opcodes.Register("cos",new ImpUnary<Basic::Cos>);
        opcodes.Register("tan",new ImpUnary<Basic::Tan>);
        opcodes.Register("asin",new ImpUnary<Basic::Asin>);
        opcodes.Register("acos",new ImpUnary<Basic::Acos>);
        opcodes.Register("atan",new ImpUnary<Basic::Atan>);
        opcodes.Register("sinh",new ImpUnary<Basic::Sinh>);
        opcodes.Register("cosh",new ImpUnary<Basic::Cosh>);
        opcodes.Register("tanh",new ImpUnary<Basic::Tanh>);

        // binary
        opcodes.Register("+",new ImpBinary<Basic::Plus>);
        opcodes.Register("-",new ImpBinary<Basic::Minus>);
        opcodes.Register("*",new ImpBinary<Basic::Times>);
        opcodes.Register("/",new ImpBinary<Basic::Over>);
        opcodes.Register("%",new ImpBinary<Basic::Mod>);
        opcodes.Register("**",new ImpBinary<Basic::Power>);

        opcodes.Register("log",new ImpBinary<Basic::Log>);
        opcodes.Register("atan2",new ImpBinary<Basic::Atan2>);

        opcodes.Register("min",new ImpBinary<Basic::Min>);
        opcodes.Register("max",new ImpBinary<Basic::Max>);

        opcodes.Register("==",new ImpBinary<Basic::Eq>);
        opcodes.Register("!=",new ImpBinary<Basic::Ne>);
        opcodes.Register("<",new ImpBinary<Basic::Lt>);
        opcodes.Register(">",new ImpBinary<Basic::Gt>);
        opcodes.Register("<=",new ImpBinary<Basic::Le>);
        opcodes.Register(">=",new ImpBinary<Basic::Ge>);

        opcodes.Register("&&",new ImpBinary<Basic::And>);
        opcodes.Register("||",new ImpBinary<Basic::Or>);
        opcodes.Register("^^",new ImpBinary<Basic::Xor>);

        opcodes.Register("&",new ImpBinary<Basic::BitAnd>);
        opcodes.Register("|",new ImpBinary<Basic::BitOr>);
        opcodes.Register("^",new ImpBinary<Basic::BitXor>);

        // vector
        opcodes.Register("sum",new OpUnary<OpVec::Sum>);
        opcodes.Register("prod",new OpUnary<OpVec::Product>);
        opcodes.Register("len",new OpUnary<OpVec::Len>);
        opcodes.Register("rev",new OpUnary<OpVec::Reverse>);

        opcodes.Register("nth",new OpBinary<OpVec::Nth>);

        opcodes.Register("packn",new OpVec::PackN);
        opcodes.Register("pack*",new OpVec::PackAll);
        opcodes.Register("unpack",new OpVec::Unpack);

        // stack
        opcodes.Register("size",new OpStack::Size);

        opcodes.Register("drop",new OpStack::Drop);
        opcodes.Register("dropn",new OpStack::DropN);
        opcodes.Register("dropi",new OpStack::DropI);
        opcodes.Register("dropni",new OpStack::DropNI);
        opcodes.Register("drop*",new OpStack::DropAll);

        opcodes.Register("dup",new OpStack::Dup);
        opcodes.Register("dupn",new OpStack::DupN);
        opcodes.Register("dupi",new OpStack::DupI);
        opcodes.Register("dupni",new OpStack::DupNI);
        opcodes.Register("dup*",new OpStack::DupAll);

        opcodes.Register("swap",new OpStack::Swap);
        opcodes.Register("swapi",new OpStack::SwapI);
        opcodes.Register("swapn",new OpStack::SwapN);
        opcodes.Register("swapni",new OpStack::SwapNI);

        opcodes.Register("rot",new OpStack::Rot);
        opcodes.Register("rotn",new OpStack::RotN);
        opcodes.Register("roti",new OpStack::RotI);
        opcodes.Register("rotni",new OpStack::RotNI);

        // inputs
        opcodes.Register("_",new OpInput::VariableI);
        opcodes.Register("_*",new OpInput::Variables);

        // memory
        opcodes.Register(">?",new OpMemory::Set);
        opcodes.Register("<?",new OpMemory::Get);

        // debug
        opcodes.Register("post",new OpDebug::Post);
        opcodes.Register("post*",new OpDebug::PostAll);
    }
};

Symbol rpn::sym_double,rpn::sym_set,rpn::sym_memory;
Symbol rpn::sym_popen,rpn::sym_pclose;
Functions rpn::global;
MemoryPool rpn::pool;
Registry<Opcode> rpn::opcodes;
Registry<Meta> rpn::metas;


FLEXT_NEW_V("rpn",rpn)
