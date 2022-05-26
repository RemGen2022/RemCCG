//==-- proto_to_c.cc - Protobuf-C++ conversion --------------------------==//
//

#include "proto_to_c.h"
#include "./build/c_proto.pb.h"

#include <ostream>
#include <sstream>
#include <src/utf8_fix.h>
#include <search.h>

void printProgram(void)
{
    FunctionList *f;
    VariableList *v;

    foreach(v, program.globalvars)
        printVariableDecl(v->variable);

}

static Context *cc ;
static bool isFunction;

int isFind(std::vector<std::string> vec, std::string name){
    for (auto i : vec){
        if (i == name) {
            return 1;
        }
    }
    return 0;
}

static std::vector<std::string> labelNameForFunction;
static std::vector<std::string> labelNameForFunctionL;
void insertStatementForFunction(Context *cc1){
   Statement *ret = makeStatementForFunction(cc1, 5, 1);
   printStatementForFunction(ret, 1);
}
static std::vector<std::string> labelName;
void insertStatement(Context *cc1){
    Block *ret1 = makeBlockForFunction(cc1, 2);
    printBlockForFunction(ret1);
}

void insertStatements(){
    insertStatement(cc);
}

void insertExpression(Context* cc_expr) {

    Expression *expr = makeExpression(cc, 15);
    printExpression(expr);
    
}

namespace clang_fuzzer {

const int array_size = 100;

// Collect the number of calling times of each statement
static int numAssignmentStmt = 0;
static int numWhileLoopStmt = 0;
static int numDowhileLoopStmt = 0;
static int numForLoopStmt = 0;
static int numIfElseStmt = 0;
static int numIfStmt = 0;
static int numSwithStmt = 0;
static int numLabeledStmt = 0;
static int numExprStmt = 0;
static int numCompStmt = 0;
static int numJumpStmt = 0;

int numlimit = 20;
static int depth = 0;
const int max_depth = 10;

std::string randome_pick_one(std::vector<std::string> vec_string){
    return vec_string[random_generate(0, vec_string.size()-1)];
}

//From ccg
static std::vector<std::string> ptrName;
static std::vector<std::string> normalVariableName;

	
// Transfer functions
std::ostream &operator<<(std::ostream &os, const Const &x) {
    insertExpression(cc);
    return os;
}

std::ostream &operator<<(std::ostream &os, const VarRef &x) {
    std::cout<< normalVariableName[random_generate(0, normalVariableName.size()-1)];
    return os;
}
std::ostream &operator<<(std::ostream &os, const Rvalue &x) {
    if (x.has_cons()){
        x.cons();
        return os;
    }
    if (x.has_binop()){
        x.binop();
        return os;
    }
    if (x.has_varref())
        std::cout<< x.varref();

    return os;
}
std::ostream &operator<<(std::ostream &os, const BinaryOp &x) {
    std::cout << "(" << x.left();
    switch (x.op()) {
        case BinaryOp::PLUS: std::cout << "+"; break;
        case BinaryOp::MINUS: std::cout << "-"; break;
        case BinaryOp::MUL: std::cout << "*"; break;
        case BinaryOp::XOR: std::cout << "^"; break;
        case BinaryOp::AND: std::cout << "&"; break;
        case BinaryOp::OR: std::cout << "|"; break;
        case BinaryOp::EQ: std::cout<< "=="; break;
        case BinaryOp::NE: std::cout << "!="; break;
        case BinaryOp::LE: std::cout << "<="; break;
        case BinaryOp::GE: std::cout << ">="; break;
        case BinaryOp::LT: std::cout << "<"; break;
        case BinaryOp::GT: std::cout << ">"; break;
  }
    std::cout << x.right() << ")";
  return os;
}

std::ostream &operator<<(std::ostream &os, const LabeledStatement &labeled_statement) {
    numLabeledStmt += 1;
    unsigned int index = random_uint();
    insertExpression(cc);
    std::string label = "L_" + std::to_string(index);
    os << labeled_statement.assignment();
    labelNameForFunctionL.push_back(label);
    return os;
}

std::ostream &operator<<(std::ostream &os, const CompoundStatement &compound_statement) {
    numCompStmt += 1;
    printf("{\n");
    insertStatement(cc);
    os << compound_statement.statements();
    printf("}\n");
    return os;
}

std::ostream &operator<<(std::ostream &os, const ExpressionStatement &expression_statement) {
    numExprStmt += 1;
    insertExpression(cc);
    printf(";\n");
    return os;
}

std::ostream &operator<<(std::ostream &os, const JumpStatement &jump_statement) {
    numJumpStmt += 1;
   
    bool j = random_generate(0,1);
    if (j == 0) {
        if (labelName.size() > 0)
            printf("goto %s;\n", labelName[random_generate(0, labelName.size()-1)].c_str());
        else
            printf("return 0;\n");
    }
    else{
        printf("return ");
        insertExpression(cc);
        printf(";\n");
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const AssignmentStatement &assignment_statement) {
    numAssignmentStmt += 1;
    if (normalVariableName.size() !=0)
        printf("%s ", normalVariableName[random_generate(0, normalVariableName.size()-1)].c_str());
    else
        printf("  =  ");
    insertExpression(cc);
    printf(";\n");
    insertStatementForFunction(cc);
    insertStatements();
    return os;
}

std::ostream &operator<<(std::ostream &os, const IfElseStatement &if_else_stmt) {
    numIfElseStmt += 1;
    depth +=1;
    if (depth > max_depth){
        return os;
    }
    printf("  if (");
    insertExpression(cc);
    printf("){\n");
    insertStatements();
        os << if_else_stmt.if_body();
    printf("}\n");
    printf("  else {\n");
    insertStatements();
    os << if_else_stmt.else_body();
    printf(" }\n");
    return os;
}

std::ostream &operator<<(std::ostream &os, const IfStatement &if_stmt) {
    numIfStmt += 1;
    depth +=1;
    if (depth > max_depth){
        return os;
    }
    printf("  if (");
    insertExpression(cc);
    printf("){\n");
    insertStatements();
    os << if_stmt.if_body().statements().assignment();
    os << if_stmt.if_body();
    printf("}\n");
    return os;
}

std::ostream &operator<<(std::ostream &os, const SwitchStatement switch_stmt){
    numSwithStmt += 1;
    depth +=1;
    if (depth > max_depth){
        return os;
    }
    //TODO change the body
    std::string var = randome_pick_one(normalVariableName);
    unsigned int index = random_uint();
    printf("    %s = %d;\n", var.c_str(), random_generate(3,10));
    printf("     switch  ");
    printf(" ( %s )", var.c_str());
    printf("{\n");
    for (int i = 0; i < 10; i++){
        printf( "    case %d :{\n", i);
            insertStatements();
            os << switch_stmt.switch_statement();
        if (random_generate(0,1)) {
            printf("break;\n");
        }
        printf("     }\n");
    }
    printf("    default:{\n");
    insertStatements();
    os << switch_stmt.switch_statement();
    if (random_generate(0,1))
		printf("break;\n");
    printf("    }}\n");

    //add orginal statement
    printf("  switch");
    printf(" (");
    insertExpression(cc);
    printf(" )");
    printf(" {\n");
    insertStatements();
    os << switch_stmt.switch_statement();
    printf(" }\n");
    return os;
}

std::ostream &operator<<(std::ostream &os, const WhileLoopStatement &while_loop_statement){
    numWhileLoopStmt += 1;
    depth +=1;
    if (depth > max_depth){
        return os;
    }
   
    std::string var = randome_pick_one(normalVariableName);
    int var_condition = random_generate(-10, 10);
    std::vector<std::string> up_operator{"+= ","+= "};
    std::vector<std::string> down_operator{"-= ","-= "};
    printf("    %s = %d;\n", var.c_str(), var_condition);
    if (var_condition > 0){
        printf("    while");
        bool mm = random_generate(0,1);
        if (mm == 1){
            printf(" ( %s -- )", var.c_str());
            printf(" {\n");
            insertStatements();
            os << while_loop_statement.statement();
            printf("    }\n");
        }else {
            printf(" ( %s  > 2){\n", var.c_str());
            printf("\n    %s  %s %d ;\n ", var.c_str(),  down_operator[random_generate(0,1)].c_str(), random_generate(2,5));
            insertStatements();
            os << while_loop_statement.statement();
            printf("    }\n");
        }
    }else if (var_condition < 0){
        printf("    while");
        bool pp = random_generate(0, 1);
        if (pp == 1){
            printf(" (%s ++ ) {\n", var.c_str());
            insertStatements();
            os << while_loop_statement.statement();
            printf("    }\n");
        }else {
            printf(" ( %s < 100){\n", var.c_str());
            printf("\n    %s %s %u ;\n ", var.c_str(),  up_operator[random_generate(0,1)].c_str(), random_generate(2,5));
            insertStatements();
            os << while_loop_statement.statement();
            printf("    }\n");
        }
    }
    else {
        printf("     while(0){");
        insertStatements();
        os << while_loop_statement.statement();
        printf("    }\n");
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const DoWhileLoopStatement &do_while_loop_statement){
    numDowhileLoopStmt += 1;
    depth +=1;
    if (depth > max_depth){
        return os;
    }
    printf("//Add do-while-loop statement depth = %d\n", depth);

    std::string var = randome_pick_one(normalVariableName);
    int var_condition = random_generate(-10,10);
    std::vector<std::string> up_operator{"+= ", "+= "}; // *=
    std::vector<std::string> down_operator{"-= ", "-= "}; // /=
    printf("    %s = %d;\n", var.c_str(), var_condition);
    if (var_condition > 0){
        printf("    do");
        bool mm = random_generate(0,1);
        if (mm == 1){
            printf(" {\n");
            insertStatements();
            os << do_while_loop_statement.statement();
            printf("    } while");
            printf(" ( %s --);\n", var.c_str());
        }else {
            printf("{\n    %s %s %d;\n ", var.c_str(), down_operator[random_generate(0,1)].c_str(), random_generate(2,5));
            insertStatements();
            os << do_while_loop_statement.statement();
            printf("    }while");
            printf(" (%s > 2 );\n", var.c_str());
        }
    }else if (var_condition < 0){
        printf("    do");
        bool pp = random_generate(0,1);
        if (pp == 1){
            printf(" {\n");
            insertStatements();
            os << do_while_loop_statement.statement();
            printf("    }while");
            printf(" ( %s ++ );\n", var.c_str());
        }else {
            printf("{\n    %s %s %u ;\n ", var.c_str(),  up_operator[random_generate(0,1)].c_str(), random_generate(2,5));
            insertStatements();
            os << do_while_loop_statement.statement();
            printf("    }while");
            printf(" ( %s < 100 );\n", var.c_str());
        }
    }
    else {
        printf("  do{\n");
        insertStatements();
        os << do_while_loop_statement.statement();
        printf("    }while( 0 );\n");
    }
    return os;

}

std::ostream &operator<<(std::ostream &os, const ForLoopStatement &for_loop_statement){
    numForLoopStmt += 1;
    depth +=1;
    if (depth > max_depth){
        return os;
    }
    

    std::string var = randome_pick_one(normalVariableName);
    int var_condition = random_generate(-10,10);
    std::vector<std::string> up_operator{"+= ", "+= "}; // *=
    std::vector<std::string> down_operator{"-=", "-="}; // /=
    printf("    %s = %d;\n", var.c_str(), var_condition);
    if (var_condition > 0){
        printf("    for");
        bool mm = random_generate(0,1);
        if (mm == 1){
            printf(" ( ; %s > 2; %s -- ){\n", var.c_str(), var.c_str()) ;
            insertStatements();
            os << for_loop_statement.statement();
        printf("    }\n");
        }else{
            printf(" ( ; %s > 2; %s %s %d ){\n ", var.c_str(), var.c_str(), down_operator[random_generate(0,1)].c_str(), random_generate(2,5));
            insertStatements();
            os << for_loop_statement.statement();
            printf("    }\n");
        }
    }else if (var_condition < 0){
        printf("    for");
        bool pp = random_generate(0,1);
        if (pp == 1){
            printf(" ( ; %s < 100; %s ++ ){\n", var.c_str(), var.c_str()) ;
            insertStatements();
            os << for_loop_statement.statement();
            printf("    }\n");
        }else {
            printf(" ( ; %s > 100; %s %s %d ){\n ", var.c_str(), var.c_str(), up_operator[random_generate(0,1)].c_str(), random_generate(2,5));
            insertStatements();
            os << for_loop_statement.statement();
            printf("    }\n");
        }
    }
    else {
        printf("    for (; 0; ){\n");
        insertStatements();
        os << for_loop_statement.statement();
        printf("    }\n");
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const Statement1 &statement) {
    int var = random_generate(1,11);
    switch (var) {
        case 1 : os << statement.assignment(); break;
        case 2 : os << statement.while_loop_statement();break;
        case 3 : os << statement.do_while_loop_statement();break;
        case 4 : os << statement.for_loop_statement(); break;
        case 5 : os  << statement.if_else_statement(); break;
        case 6 : os << statement.if_statement(); break;
        case 7 : os << statement.switch_statement(); break;
        case 10 : os << statement.labeled_statement(); break;
        case 8 : os << statement.expression_statement();;break;
        case 9: os << statement.compound_statement(); break;
        case 11: os << statement.jump_statement();break;

        default : os << statement.while_loop_statement();break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const StatementSeq_last & statement_seq_last){
    os << statement_seq_last.statement_seq();
    os << statement_seq_last.statement();
    return os;
}
std::ostream &operator<<(std::ostream &os, const StatementSeq &statement_seq) {
    int var = random_generate(1,3);
    switch (var) {
        case 1 : os << statement_seq.statements();return os;
        case 2 : os << statement_seq.statement_seq_last(); return os;
        default : os << statement_seq.statement_seq_last();return os;
    }
}

std::ostream &operator<<(std::ostream &os, const LoopFunction &loop_function) {
  
    FunctionList *f;

   
    std::vector<std::string> paramlistName;
   
    foreach(f, program.functions){
        VariableList *v;
        foreach(v, f->function->paramlist){
            std::string var_name = v->variable->name;
            paramlistName.push_back(var_name);
        }
	}

    foreach(f, program.functions){
        VariableList *v;

        Block* block = f->function->body;
        foreach(v, block->localvars){
            std::string name = v->variable->name;
            if (v->variable->type != _integer){
                ptrName.push_back(name);
            }
        }
    }
    printf("\nint loop_func(){\n");
   
    Statement *ret_flag = makeStatement(cc, 8, 1);
    printStatementForFunction(ret_flag, 0); //change the flag

    //remove dup
    sort(paramlistName.begin(), paramlistName.end());
    paramlistName.erase(unique(paramlistName.begin(), paramlistName.end()), paramlistName.end());

    sort(ptrName.begin(), ptrName.end());
    ptrName.erase(unique(ptrName.begin(), ptrName.end()), ptrName.end());

    for (auto it : paramlistName){
        std::string type = randome_pick_one(simpleType);
        printf("%s %s;\n", type.c_str(), it.c_str());
    }
    for (auto it : ptrName){
    }
    //print function body
    foreach(f, program.functions){
        VariableList *v;

        StatementList *s;
        Block* block = f->function->body;
        foreach(v, block->localvars){
            std::string name = v->variable->name;
            int findParam = isFind(paramlistName, name);
            int findPtr = isFind(ptrName, name);
            if (v->variable->type == _integer){
                if (findParam == 0){
                    if (! isFind(normalVariableName, name)){
                        printIntegerDecl(v->variable);
                        normalVariableName.push_back(name);
                    }
                }
            }
            else {
                if ( findPtr == 1 ) {
                    printPointerDecl(v->variable);
                    ptrName.erase(std::remove(ptrName.begin(), ptrName.end(), name), ptrName.end());
                }
            }
        }

        foreach(s, block->statementlist){
            printStatement(s->statement);
	    }
    }
    
    //clear the paramlistName set
    paramlistName.clear();

    insertStatements();

    
    printf("\nreturn 0;\n}\n");
    normalVariableName.clear();
    labelName.clear();
    return os;
}

std::ostream &operator<<(std::ostream &os, const MainFunction &main_function) {
    puts("\n\n");
    printf("int main(){\n");
    printf("    func_0();\n");
    printf("}\n");

    return os;
}
// ---------------------------------

std::string LoopFunctionToString(const LoopFunction &loop_function) {
    std::ostringstream os;
    os << loop_function;
    return os.str();
}

std::vector<int> MainFunctionToString(const MainFunction &main_function, unsigned seed_ccg) {
    std::ostringstream os;
    std::vector<int> retVec;
    //clear the former variables
    retVec.clear();
    numAssignmentStmt = 0;
    numWhileLoopStmt = 0;
    numDowhileLoopStmt = 0;
    numForLoopStmt = 0;
    numIfElseStmt = 0;
    numIfStmt = 0;
    numSwithStmt = 0;
    numLabeledStmt = 0;
    numCompStmt = 0;
    numExprStmt = 0;
    numJumpStmt = 0;
    std::vector<std::string> funcName;
    cc = (Context*) xmalloc(sizeof(*cc));
  
    program.globalvars = NULL, program.functions = NULL;
    program.numfunctions = program.numglobalvars = 0;

    /* Default settings */
    cmdline.max_functions = 10; //10 
    cmdline.max_localvars = 4; //4
    cmdline.max_function_parameters = 4; //4
    cmdline.max_statements_per_block = 7; //7
    cmdline.max_expression_nesting = 8; //8
    cmdline.max_block_nesting = 7; //7
    cmdline.max_pointer_depth = 2; //2

    cmdline.seed = seed_ccg;
    cmdline.nojumps = false;

    int argcc = 1;
    char *argvv[] = {"./ccg"};
    processCommandline(argcc, argvv);
    srand(cmdline.seed);
    makeGlobalVariables();


    puts("#include <stdint.h>");
    puts("#include <stdarg.h>");
    puts("#include <stdlib.h>\n\n/* Global variables */");
   
    VariableList *v;
    foreach(v, program.globalvars) {
        printVariableDecl(v->variable) ;
        std::string name = v->variable->name;
        normalVariableName.push_back(name);
    }

    printf("int aaa;\n");
    cc->scope = program.globalvars;
    cc->nvars = program.numglobalvars;
    cc->nintegers = program.numglobalvars;


    Function *func = makeFunction(false);
   
    cc->currfunc = func;

    int nesting = 8;
    Context *context = cc;
    size_t i, numlocalvars, numstatements;
    Block *ret = (Block *) xmalloc(sizeof(*ret));
    Context *bcontext = (Context *) xmalloc(sizeof(*bcontext));

    bcontext->currfunc = context->currfunc;
    bcontext->nvars = context->nvars;
    bcontext->nintegers = context->nintegers;
    bcontext->scope = NULL;

    copyVariableList(context->scope, &bcontext->scope);

    ret->localvars = NULL, ret->statementlist = NULL;

    numlocalvars = rand() % cmdline.max_localvars + !nesting;

    for(i = 0; i < numlocalvars; ++i)
    {
        Variable *tmp = makeVariable(bcontext, _randomvartype);
        bcontext->nvars++;

        if(tmp->type == _integer)
            bcontext->nintegers++;

        addVariableToList(tmp, &(ret->localvars));
        addVariableToList(tmp, &bcontext->scope);
    }

    if(nesting < cmdline.max_block_nesting)
    {
        numstatements = rand() % cmdline.max_statements_per_block;

        for(i = 0; i < numstatements; ++i)
        {
            Statement *tmp = makeStatement(bcontext, nesting, !(numstatements - 1 - i));
	    if (tmp->label){printf("/*label name: %s*/\n", tmp->label->name);}
            addStatementToList(tmp, &ret->statementlist);
        }
    }

    freeVariableList(bcontext->scope);
    free(bcontext);


    FunctionList *f;
    foreach(f, program.functions){
        std::string name = f->function->name;
        if (! isFind(funcName, name)) {
            printFunctionPrototype(f->function);
            printf(";\n");
            funcName.push_back(name);
       }
    }
    funcName.clear();

    os << "\n";
    foreach(f, program.functions){
        std::string name = f->function->name;
    }
    Context *temp_cc = (Context*) xmalloc(sizeof(*temp_cc));
    temp_cc->nvars = cc->nvars;
    temp_cc->nintegers = cc->nintegers;
    temp_cc->scope = cc->scope;
    temp_cc->currfunc = cc->currfunc;
    foreach(f, program.functions){
        std::string name = f->function->name;
        if (! isFind(funcName, name)) {
            printFunctionPrototype(f->function);
            printf("\n");
            printf("{\n");
            Context *lcc = (Context*) xmalloc(sizeof(*lcc));
            lcc->nvars = f->function->nvars;
            lcc->nintegers = f->function->nintegers;
            lcc->scope = f->function->scope;
            lcc->currfunc = f->function->currfunc;

            MainFunction ma;
            isFunction = 1;
           
            cc->nvars = lcc->nvars;
            cc->nintegers = lcc->nintegers;
            cc->scope = lcc->scope;
            cc->currfunc = lcc->currfunc;

            VariableList *v;
            StatementList *s;
            foreach(v, f->function->body->localvars) {
                printVariableDecl(v->variable);
            }

            foreach(s, f->function->body->statementlist) {
                printStatement(s->statement);
                    int var = random_generate(1,11);
                    switch (var){
                        printf("//inserted statements start\n");
                        case 1: os << ma.loop_function().statement_seq().statements().assignment(); break;
                        case 2: os << ma.loop_function().statement_seq().statements().while_loop_statement(); depth = 0; break;
                        case 3: os << ma.loop_function().statement_seq().statements().do_while_loop_statement(); depth = 0; break;
                        case 4: os << ma.loop_function().statement_seq().statements().for_loop_statement(); depth = 0; break;
                        case 5: os << ma.loop_function().statement_seq().statements().if_else_statement(); depth = 0; break;
                        case 6: os << ma.loop_function().statement_seq().statements().if_statement(); depth = 0; break;
                        case 7: os << ma.loop_function().statement_seq().statements().switch_statement(); depth = 0; break;
                        case 8: os << ma.loop_function().statement_seq().statements().labeled_statement(); break;
                        case 9: os << ma.loop_function().statement_seq().statements().expression_statement(); break;
                        case 10: os << ma.loop_function().statement_seq().statements().compound_statement(); break;
                        case 11: os << ma.loop_function().statement_seq().statements().jump_statement(); depth = 0; break;
                        default: os << ma.loop_function().statement_seq().statements().while_loop_statement(); depth = 0; break;

                    }

            }
            printf("}\n");
            funcName.push_back(name);

            isFunction = 0; 
       }
       
    }
    cc->nvars = temp_cc->nvars;
    cc->nintegers = temp_cc->nintegers;
    cc->scope = temp_cc->scope;
    cc->currfunc = temp_cc->currfunc;
    free(temp_cc);

    os << main_function;
    
   
    free(cc);
    /*
    printf("//Statictics of gramar coverage\n");
    printf("//numAssignmentStmt = %d\n", numAssignmentStmt);
    printf("//numWhileLoopStmt = %d\n", numWhileLoopStmt);
    printf("//numDowhileLoopStmt = %d\n", numDowhileLoopStmt);
    printf("//numForLoopStmt = %d\n", numForLoopStmt);
    printf("//numIfElseStmt = %d\n", numIfElseStmt);
    printf("//numIfStmt = %d\n", numIfStmt);
    printf("//numSwithStmt = %d\n", numSwithStmt);
    printf("//numLabeledStmt = %d\n", numLabeledStmt);
    printf("//numCompStmt = %d\n", numCompStmt);
    printf("//numExprStmt = %d\n", numExprStmt);
    printf("//numJumpStmt = %d\n", numJumpStmt);
    */
	
    //Add the above numbers to the retVec
    retVec.push_back(numAssignmentStmt);
    retVec.push_back(numWhileLoopStmt);
    retVec.push_back(numDowhileLoopStmt);
    retVec.push_back(numForLoopStmt);
    retVec.push_back(numIfElseStmt);
    retVec.push_back(numIfStmt);
    retVec.push_back(numSwithStmt);
    retVec.push_back(numLabeledStmt);
    retVec.push_back(numCompStmt);
    retVec.push_back(numExprStmt);
    retVec.push_back(numJumpStmt);

    //clear the previous label name, to make sure the following label can be printed
    rootMakeLabel = 0;
    root = 0;

    //clear the variable name list
    normalVariableName.clear();
    return retVec;
}
	
} // namespace clang_fuzzer_loop

