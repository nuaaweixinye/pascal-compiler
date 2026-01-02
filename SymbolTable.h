#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

// ========== 枚举定义（强类型枚举，C++11+） ==========
// 符号类型
enum class SYMBOLTYPE {
    Const,  // 常量
    VAR,    // 变量
    PARAM,  // 过程参数
    PROC    // 过程
};

// 语义错误类型
enum class SymErrType {
    DUP_DEF,        // 同层重复定义
    UNDEF,          // 未定义标识符
    TYPE_MISMATCH,  // 类型不匹配
    PARAM_MISMATCH  // 过程参数个数不匹配
};

// 语义错误异常（C++异常处理）
class SymbolError : public  runtime_error {
public:
    SymbolError(SymErrType err_type, const  string& name)
        : runtime_error(generateMsg(err_type, name)),
        err_type_(err_type), name_(name), line_(0) { // 补充line_初始化，避免未初始化警告
    }

    // 获取错误详情
    SymErrType getErrType() const { return err_type_; }
    string getName() const { return name_; }
    int getLine() const { return line_; }

private:
    SymErrType err_type_;
    string name_;
    int line_;

    // 生成错误信息
    string generateMsg(SymErrType err_type, const  string& name) {
        string msg = "";
        switch (err_type) {
        case SymErrType::DUP_DEF:
            msg += "标识符\"" + name + "\"在当前层重复定义";
            break;
        case SymErrType::UNDEF:
            msg += "标识符\"" + name + "\"未定义";
            break;
        case SymErrType::TYPE_MISMATCH:
            msg += "标识符\"" + name + "\"类型不匹配";
            break;
        case SymErrType::PARAM_MISMATCH:
            msg += "过程\"" + name + "\"调用参数个数不匹配";
            break;
        }
        return msg;
    }
};

class SymLayer;

// ========== 符号项类（Symbol） ==========
class Symbol {

public:
    // 通用属性
    string name_;
    SYMBOLTYPE type_;
    int level_;
    Symbol* next_; 

    // 专属属性（共用体，C++11+支持非平凡类型）
    union Attr {
        int const_val;                // 常量值
        struct VarAttr {
			int offset;              // 变量/参数偏移
			int value;			   // 变量值
        } var_attr;             
        struct ProcAttr {             // 过程属性
            int param_count;// 参数个数
            int var_count; // 局部变量个数
			int entry_addr;//pcode入口地址
			SymLayer* layer_ptr; // 过程对应的符号层指针（可选，便于访问过程内符号）
        } proc_attr;

        Attr() : const_val(0) {}      // 共用体构造
        ~Attr() {}
    } attr_;

    // 构造函数（按符号类型重载）
    Symbol(const  string& name, SYMBOLTYPE type, int level)
        : name_(name), type_(type), level_(level), next_(nullptr) {
    }

    // 常量符号构造
    static  Symbol* createConst(const  string& name, int level, int val) {
        Symbol* sym = new Symbol(name, SYMBOLTYPE::Const, level);
        sym->attr_.const_val = val;
        return sym;
    }

    // 变量/参数符号构造
    static  Symbol* createVarOrParam(const  string& name, SYMBOLTYPE type, int level, int offset , int value) {
        if (type != SYMBOLTYPE::VAR && type != SYMBOLTYPE::PARAM) {
            throw  invalid_argument("类型必须是VAR或PARAM");
        }
        Symbol* sym = new Symbol(name, type, level);
		sym->attr_.var_attr.offset = offset;
		sym->attr_.var_attr.value = value;
        return sym;
    }

    // 过程符号构造
    static  Symbol* createProc(const  string& name, int level, int param_count, int entry_addr = -1) {
        Symbol* sym = new Symbol(name, SYMBOLTYPE::PROC, level);
        sym->attr_.proc_attr.param_count = param_count;
        sym->attr_.proc_attr.entry_addr = entry_addr;
        return sym;
    }

    // 获取属性（只读，封装性）
    string getName() const { return name_; }
    SYMBOLTYPE getType() const { return type_; }
    int getLevel() const { return level_; }
    int getConstVal() const {
        if (type_ != SYMBOLTYPE::Const) {
            cerr << "错误：符号 " << name_ << " 不是常量，无法获取常量值" << endl;
            return 0; // 返回默认值避免程序崩溃
        }
        return attr_.const_val;
    }
    int getOffset() const {
        if (type_ != SYMBOLTYPE::VAR && type_ != SYMBOLTYPE::PARAM) {
            cerr << "错误：符号 " << name_ << " 不是变量或参数，无法获取偏移量" << endl;
            return 0; // 返回默认值避免程序崩溃
        }
		return attr_.var_attr.offset;
    }
    int getProcParamCount() const {
        if (type_ != SYMBOLTYPE::PROC) {
            cerr << "错误：符号 " << name_ << " 不是过程，无法获取参数个数" << endl;
            return 0; // 返回默认值避免程序崩溃
        }
        return attr_.proc_attr.param_count;
    }
    int getProcEntryAddr() const {
        if (type_ != SYMBOLTYPE::PROC) {
            cerr << "错误：符号 " << name_ << " 不是过程，无法获取入口地址" << endl;
            return -1; // 返回默认值避免程序崩溃
        }
        return attr_.proc_attr.entry_addr;
    }
    int getValue() {
        if (type_ == SYMBOLTYPE::VAR || type_ == SYMBOLTYPE::PARAM) {
            return attr_.var_attr.value;
        }
        else if (type_ == SYMBOLTYPE::Const) {
            return attr_.const_val;
        }
        else {
            cerr << "错误：符号 " << name_ << " 类型不支持获取值操作" << endl;
            return 0; // 返回默认值避免程序崩溃
        }
    }

    //设置过程变量个数
    void setProcVarCount(int var_count) {
        if (type_ != SYMBOLTYPE::PROC) {
            cerr << "错误：符号 " << name_ << " 不是过程，无法设置变量个数" << endl;
            return;
        }
        attr_.proc_attr.var_count = var_count;
    }

    // 设置过程入口地址（P代码生成时回填）
    void setProcEntryAddr(int addr) {
        cerr << "错误：符号 " << name_ << " 不是过程，无法设置入口地址" << endl;
        return;
    }

    // 链表指针（层内符号链表）
    Symbol* getNext() const { return next_; }
    void setNext(Symbol* next) { next_ = next; }
};

// ========== 符号表层类（SymLayer） ==========
class SymLayer {    

public:

    string layer_name_ = "";         //当前层的过程名
    int level_;               // 层级
    SymLayer* outer_;         // 外层指针（裸指针，由SymbolTable管理）
    Symbol* sym_head_;        // 层内符号链表头
	Symbol* sym_tail_;        // 层内符号链表尾
    int var_offset_;          // 变量偏移计数器
    int param_count_;         // 参数计数器


    SymLayer(int level, SymLayer* outer)
        : level_(level), outer_(outer),  var_offset_(0), param_count_(0), layer_name_("") {
		sym_head_ = sym_tail_ = nullptr;// 初始化符号链表为空
    }

    ~SymLayer() {
        // 销毁层内符号链表
        Symbol* cur = sym_head_;
        while (cur != nullptr) {
            Symbol* tmp = cur;
            cur = cur->getNext();
            delete tmp;  // 释放符号节点（手动释放，对应create系列的new）
        }
    }

    string getLayerName() {
        return layer_name_;
    }
    void setLayerName(string layer_name) {
        this->layer_name_ = layer_name;
    }

    // 查找层内符号（LL(1)无回溯查找）
    Symbol* findInLayer(const  string name) const {
        Symbol* cur = sym_head_;
        while (cur != nullptr) {
            if (cur->getName() == name) {
                return cur;
            }
            cur = cur->getNext();
        }
        return nullptr;
    }

    // 插入符号到尾
    void insertSymbol(Symbol* sym) {
        if (sym == nullptr) return;
        
        if (sym_head_ == nullptr) {
            sym_head_ = sym;
            sym_tail_ = sym;
        }
        else {
            sym_tail_->setNext(sym);
            sym_tail_ = sym;
		}
    }

    // 属性访问
    int getLevel()  { return level_; }
    SymLayer* getOuter()  { return outer_; }
    int getVarOffset()  { return var_offset_; }
    int getParamCount()  { return param_count_; }

    // 偏移量/参数计数自增
    int incVarOffset() { return var_offset_++; }
    int incParamCount() { return param_count_++; }

    // 打印层内符号（调试）
    void printLayer()  {
        cout << "  层级" << level_ << "：" << endl;
        if (sym_head_ == nullptr) {
            cout << "    无符号" << endl;
            return;
        }
        Symbol* cur = sym_head_;
        while (cur != nullptr) {
            cout << "    名称：" << cur->getName() << " | 类型：";

            SYMBOLTYPE ty = cur->getType();
            if (ty == SYMBOLTYPE::Const) {
                cout << "常量 | 值：" << cur->getConstVal();
            }
            else if (ty == SYMBOLTYPE::VAR) {
                cout << "变量 | 偏移：" << cur->getOffset(); 
            }
            else if (ty == SYMBOLTYPE::PARAM) {
                cout << "参数 | 偏移：" << cur->getOffset();
            }
            else if (ty == SYMBOLTYPE::PROC) {
                cout << "过程 | 参数数：" << cur->getProcParamCount()
                    << " | 入口地址：" << cur->getProcEntryAddr();
            }

            cout << endl;
            cur = cur->getNext();
        }
    }
};

// ========== 符号表管理器类（SymbolTable） ==========
class SymbolTable {
public:
	SymLayer* first_layer_ ;//最外层符号层指针

    SymLayer* current_layer_;// 当前符号层指针

    SymbolTable() :current_layer_(nullptr) {
        first_layer_=current_layer_ = new SymLayer(0, nullptr);
    }

    ~SymbolTable() {
        // 手动递归释放所有符号层（替代智能指针自动释放）
        releaseAllLayers(current_layer_);
    }



    // ========== 符号插入API ==========
    void insertConst(const  string& name, int val) {
        // 检查重复定义
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name);
        }
        // 创建并插入常量符号
        Symbol* sym = Symbol::createConst(name, current_layer_->getLevel(), val);
        current_layer_->insertSymbol(sym);
    }
    void insertVar(string name , int val = 0) {
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name);
        }
        // 自动分配偏移量
        int offset = current_layer_->incVarOffset();
        Symbol* sym = Symbol::createVarOrParam(name, SYMBOLTYPE::VAR, current_layer_->getLevel(), offset ,val);
        current_layer_->insertSymbol(sym);
    }
    void insertParam(string name , int val = 0) {
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name);
        }
        // 参数偏移从0开始
        int offset = current_layer_->incParamCount();
        // 变量偏移从参数个数开始
        current_layer_->incVarOffset();  // var_offset = param_count
        Symbol* sym = Symbol::createVarOrParam(name, SYMBOLTYPE::PARAM, current_layer_->getLevel(), offset ,val);
        current_layer_->insertSymbol(sym);
    }
    Symbol* insertProc(string name, int param_count = 0, int entry_addr = -1) {
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name);
        }
        Symbol* sym = Symbol::createProc(name, current_layer_->getLevel(), param_count, entry_addr);
        current_layer_->insertSymbol(sym);
		return sym;
    }




    // ========== 符号查找API（适配LL(1)文法） ==========
    // 全局查找，返回符号+层差  ,层差 = 调用层 - 定义层
    Symbol* findGlobal(const  string name, int& level_diff ,int used_level = 0) {
        SymLayer* cur_layer = current_layer_; 
		SymLayer* layer = first_layer_;
		vector<SymLayer*> layer_stack;
		layer_stack.push_back(first_layer_);
		//从最外层开始查找
        while (!layer_stack.empty())
        {
			layer = layer_stack.front();
			layer_stack.erase(layer_stack.begin());
			// 查找当前层内符号
			Symbol* sym = layer->sym_head_;
            while (sym != nullptr) {
                if (sym->getName() == name) {
                    level_diff = used_level - layer->getLevel();
                    return sym;
                }
                else if (sym->getType() == SYMBOLTYPE::PROC && sym->attr_.proc_attr.layer_ptr != nullptr) {
					layer_stack.push_back(sym->attr_.proc_attr.layer_ptr);
                }
				sym = sym->getNext();
                
            }

        }
        // 未找到则抛异常
        throw SymbolError(SymErrType::UNDEF, name);
    }

    //寻找当前过程在上一层的定义
    Symbol* findProc() {
        Symbol* sym = findGlobal(current_layer_->getLayerName(), *(new int));
        return sym;
    }

    //根据入口地址查找过程符号
    SymLayer* findProcByEntry(int entry_addr) {
        // 从最外层开始层级遍历（广度优先/层序）
        vector<SymLayer*> layers;
        layers.push_back(first_layer_);

        while (!layers.empty()) {
            SymLayer* layer = layers.front();
            layers.erase(layers.begin());
            if (layer == nullptr) continue;

            Symbol* sym = layer->sym_head_;
            while (sym != nullptr) {
                // 找到过程且入口地址匹配，返回该过程对应的内层 SymLayer 指针（可能为 nullptr）
                if (sym->getType() == SYMBOLTYPE::PROC && sym->getProcEntryAddr() == entry_addr) {
                    return sym->attr_.proc_attr.layer_ptr;
                }
                // 若是过程且有内层，则把内层加入遍历队列
                if (sym->getType() == SYMBOLTYPE::PROC && sym->attr_.proc_attr.layer_ptr != nullptr) {
                    layers.push_back(sym->attr_.proc_attr.layer_ptr);
                }
                sym = sym->getNext();
            }
        }

        // 未找到则返回 nullptr（调用方可根据需要抛错或处理）
		cerr << "警告：未找到入口地址为 " << entry_addr << " 的过程符号" << endl;
        return nullptr;
    }


    // ========== 作用域管理（适配<proc>的内层/外层切换） ==========
    // 进入过程内层
    SymLayer* enterProcLayer() {

        SymLayer* new_layer = new SymLayer(current_layer_->getLevel()+1, current_layer_);
        current_layer_ = new_layer;
        return current_layer_;
    }

    // 退出过程内层
    void exitProcLayer() {
        if (current_layer_->getLevel() == 0) {
            throw  runtime_error("无法退出全局层");
        }

        SymLayer* outer = current_layer_->getOuter();
        SymLayer* current = current_layer_;
        current_layer_ = outer;  // 切换到外层

    }


    // ========== 辅助API ==========

    //回填变量个数（过程名在上一级）
    void fillProcVarCount(int var_count) {
        
		Symbol* cur_sym = findProc();
        // 回填变量个数
        cur_sym->setProcVarCount(var_count);
    }

    // 仅在本层内，从后向前查找未回填入口地址的过程符号（用于过程嵌套结束回填）
    string findNearestUnfilledProc() {
        // 1. 仅限定当前层，不向外层遍历
        SymLayer* current_only_layer = current_layer_;
        if (current_only_layer == nullptr) {
            throw runtime_error("当前符号表层不存在，无法查找过程符号");
        }

        Symbol* cur_sym = current_only_layer->sym_head_;
        // 2. 先遍历到链表尾部（获取尾节点，实现从后向前查找的基础）
        // 特殊处理：当前层无符号（链表为空）
        if (cur_sym == nullptr) {
            throw runtime_error("当前层无任何符号，未找到需要回填的过程符号");
        }

        // 用于保存链表节点的前驱，实现从后向前遍历
        vector<Symbol*> symbol_list;
        // 先将当前层所有符号节点存入vector，按链表顺序（头→尾）存储
        while (cur_sym != nullptr) {
            symbol_list.push_back(cur_sym);
            cur_sym = cur_sym->getNext();
        }

        // 3. 逆序遍历vector（即链表的尾→头，从后向前查找）
        for (auto it = symbol_list.rbegin(); it != symbol_list.rend(); ++it) {
            Symbol* sym = *it;
            // 筛选未回填入口地址的过程符号
            if (sym->getType() == SYMBOLTYPE::PROC &&
                sym->getProcEntryAddr() == -1) {
                return sym->getName();
            }
        }

        // 4. 未找到则抛出异常
        throw runtime_error("当前层内未找到需要回填入口地址的过程符号");
    }
    
    // 回填过程入口地址，过程嵌套结束回填
    void fillProcEntry(int entry_addr) {

        int level_diff = 0;
        string name = findNearestUnfilledProc();
        Symbol* proc_sym = findGlobal(name, level_diff);
        if (proc_sym->getType() != SYMBOLTYPE::PROC) {
            throw SymbolError(SymErrType::TYPE_MISMATCH, name);
        }
        proc_sym->setProcEntryAddr(entry_addr);
    }


    // 检查过程参数个数匹配
    void checkParamCount(string proc_name, int arg_count) {
        int level_diff = 0;
        Symbol* proc_sym = findGlobal(proc_name, level_diff);
        if (proc_sym->getType() != SYMBOLTYPE::PROC) {
            throw SymbolError(SymErrType::TYPE_MISMATCH, proc_name);
        }
        if (arg_count != proc_sym->getProcParamCount()) {
            throw SymbolError(SymErrType::PARAM_MISMATCH, proc_name);
        }
    }

    // 打印整个符号表（调试）
    void printTable() const {
        cout << "\n===== PL/0 符号表=====" << endl;
        // 遍历所有层（从当前层到全局层）
        SymLayer* cur_layer;
		//从最外层开始
		vector<SymLayer*> layers;
		layers.push_back(first_layer_);
		//层级遍历打印
        while (!layers.empty()) {
            cur_layer = layers.front();
            layers.erase(layers.begin());
            if (cur_layer != nullptr) {
				cout << "========过程：" << cur_layer->getLayerName() << " | 层级" << cur_layer->getLevel() << "========" << endl;
				Symbol* sym = cur_layer->sym_head_;
				// 打印层信息
                while (sym != nullptr) {
					// 打印符号信息
                    SYMBOLTYPE ty = sym->getType();
                    if (ty == SYMBOLTYPE::Const) {
                        cout << "常量 | 值：" << sym->getConstVal();
                    }
                    else if (ty == SYMBOLTYPE::VAR) {
                        cout << "变量"<<sym->getName() <<" | 偏移：" << sym->getOffset(); // 修正原代码的VAR/PARAM打印混淆
                    }
                    else if (ty == SYMBOLTYPE::PARAM) {
                        cout << "参数" << sym->getName() << " | 偏移：" << sym->getOffset();
                    }
                    else if (ty == SYMBOLTYPE::PROC) {
                        cout << "过程 " << sym->getName() << " | 参数数：" << sym->getProcParamCount()
                            << " | 入口地址：" << sym->getProcEntryAddr();
						layers.push_back(sym->attr_.proc_attr.layer_ptr);//加入过程层，后续打印
                    }
                    cout << endl;
					sym = sym->getNext();
                }
            }
			
        }

        
    }


private:

    // 递归释放所有符号层（辅助析构函数）
    void releaseAllLayers(SymLayer* layer) {
        if (layer == nullptr) return;
        // 先递归释放外层（避免悬挂指针）
        releaseAllLayers(layer->getOuter());
        // 释放当前层（SymLayer析构函数会自动释放内部符号链表）
        delete layer;
    }
};