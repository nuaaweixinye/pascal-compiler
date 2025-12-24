#pragma once
#include <iostream>
#include <string>
#include <memory>
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
    SymbolError(SymErrType err_type, const  string& name, int line)
        :  runtime_error(generateMsg(err_type, name, line)),
        err_type_(err_type), name_(name), line_(line) {
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
     string generateMsg(SymErrType err_type, const  string& name, int line) {
         string msg = "[行" +  to_string(line) + "] ";
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


// ========== 符号项类（Symbol） ==========
class Symbol {

private:
    // 通用属性
    string name_;
    SYMBOLTYPE type_;
    int level_;
    Symbol* next_;  // 裸指针（链表节点，由层管理生命周期）

    // 专属属性（共用体，C++11+支持非平凡类型）
    union Attr {
        int const_val;                // 常量值
        int offset;                   // 变量/参数偏移
        struct ProcAttr {             // 过程属性
            int param_count;
            int entry_addr;
        } proc_attr;

        Attr() : const_val(0) {}      // 共用体构造
        ~Attr() {}
    } attr_;




public:
    // 构造函数（按符号类型重载）
    Symbol(const  string& name, SYMBOLTYPE type, int level)
        : name_(name), type_(type), level_(level), next_(nullptr) {
    }

    // 常量符号构造
    static  unique_ptr<Symbol> createConst(const  string& name, int level, int val) {
        auto sym =  make_unique<Symbol>(name, SYMBOLTYPE::Const, level);
        sym->attr_.const_val = val;
        return sym;
    }

    // 变量/参数符号构造
    static  unique_ptr<Symbol> createVarOrParam(const  string& name, SYMBOLTYPE type, int level, int offset) {
        if (type != SYMBOLTYPE::VAR && type != SYMBOLTYPE::PARAM) {
            throw  invalid_argument("类型必须是VAR或PARAM");
        }
        auto sym =  make_unique<Symbol>(name, type, level);
        sym->attr_.offset = offset;
        return sym;
    }

    // 过程符号构造
    static  unique_ptr<Symbol> createProc(const  string& name, int level, int param_count, int entry_addr = -1) {
        auto sym =  make_unique<Symbol>(name, SYMBOLTYPE::PROC, level);
        sym->attr_.proc_attr.param_count = param_count;
        sym->attr_.proc_attr.entry_addr = entry_addr;
        return sym;
    }

    // 获取属性（只读，封装性）
    string getName() const { return name_; }
    SYMBOLTYPE getType() const { return type_; }
    int getLevel() const { return level_; }
    int getConstVal() const {
        if (type_ != SYMBOLTYPE::Const) throw SymbolError(SymErrType::TYPE_MISMATCH, name_, 0);
        return attr_.const_val;
    }
    int getOffset() const {
        if (type_ != SYMBOLTYPE::VAR && type_ != SYMBOLTYPE::PARAM)
            throw SymbolError(SymErrType::TYPE_MISMATCH, name_, 0);
        return attr_.offset;
    }
    int getProcParamCount() const {
        if (type_ != SYMBOLTYPE::PROC) throw SymbolError(SymErrType::TYPE_MISMATCH, name_, 0);
        return attr_.proc_attr.param_count;
    }
    int getProcEntryAddr() const {
        if (type_ != SYMBOLTYPE::PROC) throw SymbolError(SymErrType::TYPE_MISMATCH, name_, 0);
        return attr_.proc_attr.entry_addr;
    }

    // 设置过程入口地址（P代码生成时回填）
    void setProcEntryAddr(int addr) {
        if (type_ != SYMBOLTYPE::PROC) throw SymbolError(SymErrType::TYPE_MISMATCH, name_, 0);
        attr_.proc_attr.entry_addr = addr;
    }

    // 链表指针（层内符号链表）
    Symbol* getNext() const { return next_; }
    void setNext(Symbol* next) { next_ = next; }


};

// ========== 符号表层类（SymLayer） ==========
class SymLayer {

private:
    int level_;               // 层级
    SymLayer* outer_;         // 外层指针（裸指针，由SymbolTable管理）
    Symbol* sym_head_;        // 层内符号链表头
    int var_offset_;          // 变量偏移计数器
    int param_count_;         // 参数计数器



public:
    SymLayer(int level, SymLayer* outer)
        : level_(level), outer_(outer), sym_head_(nullptr), var_offset_(0), param_count_(0) {
    }

    ~SymLayer() {
        // 销毁层内符号链表
        Symbol* cur = sym_head_;
        while (cur != nullptr) {
            Symbol* tmp = cur;
            cur = cur->getNext();
            delete tmp;  // 释放符号节点
        }
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

    // 插入符号到层（头插法，LL(1)高效）
    void insertSymbol( unique_ptr<Symbol> sym) {
        if (sym == nullptr) return;
        sym->setNext(sym_head_);
        sym_head_ = sym.release();  // 移交所有权到层链表
    }

    // 属性访问
    int getLevel() const { return level_; }
    SymLayer* getOuter() const { return outer_; }
    int getVarOffset() const { return var_offset_; }
    int getParamCount() const { return param_count_; }

    // 偏移量/参数计数自增
    int incVarOffset() { return var_offset_++; }
    int incParamCount() { return param_count_++; }

    // 打印层内符号（调试）
    void printLayer() const {
         cout << "  层级" << level_ << "：" <<  endl;
        if (sym_head_ == nullptr) {
             cout << "    无符号" <<  endl;
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
                cout << "参数 | 偏移：" << cur->getOffset();
            }
            else if (ty == SYMBOLTYPE::PARAM) {
                cout << "变量 | 偏移：" << cur->getOffset();
            }
            else if (ty == SYMBOLTYPE::PROC) {
                cout << "过程 | 参数数：" << cur->getProcParamCount()
                    << " | 入口地址：" << cur->getProcEntryAddr();
			}

            cout <<  endl;
            cur = cur->getNext();
        }
    }

};

// ========== 符号表管理器类（SymbolTable） ==========
class SymbolTable {
public:
    SymbolTable() : total_level_(0), current_line_(1), err_flag_(false) {
        // 初始化全局层（层级0）
        current_layer_ =  make_unique<SymLayer>(0, nullptr);
    }

    ~SymbolTable() = default;  // 智能指针自动释放层

    // ========== 符号插入API（适配LL(1)文法） ==========
    // 插入常量（<const>解析时调用）
    void insertConst(const  string& name, int val) {
        // 检查重复定义
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name, current_line_);
        }
        // 创建并插入常量符号
        auto sym = Symbol::createConst(name, current_layer_->getLevel(), val);
        current_layer_->insertSymbol( move(sym));
    }

    // 插入变量（<vardecl>解析时调用）
    void insertVar(const  string& name) {
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name, current_line_);
        }
        // 自动分配偏移量
        int offset = current_layer_->incVarOffset();
        auto sym = Symbol::createVarOrParam(name, SYMBOLTYPE::VAR, current_layer_->getLevel(), offset);
        current_layer_->insertSymbol( move(sym));
    }


    // 插入参数（<param_list_opt>解析时调用）
    void insertParam(const  string& name) {
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name, current_line_);
        }
        // 参数偏移从0开始
        int offset = current_layer_->incParamCount();
        // 变量偏移从参数个数开始
        current_layer_->incVarOffset();  // var_offset = param_count
        auto sym = Symbol::createVarOrParam(name, SYMBOLTYPE::PARAM, current_layer_->getLevel(), offset);
        current_layer_->insertSymbol( move(sym));
    }

    // 插入过程（<proc>解析时调用）
    void insertProc(const  string& name, int param_count, int entry_addr = -1) {
        if (current_layer_->findInLayer(name) != nullptr) {
            throw SymbolError(SymErrType::DUP_DEF, name, current_line_);
        }
        auto sym = Symbol::createProc(name, current_layer_->getLevel(), param_count, entry_addr);
        current_layer_->insertSymbol( move(sym));
    }

    // ========== 符号查找API（适配LL(1)文法） ==========
    // 全局查找（从当前层到外层），返回符号+层差
    Symbol* findGlobal(const  string name, int& level_diff) {
        SymLayer* cur_layer = current_layer_.get();//返回智能指针所管理对象的裸指针（raw pointer）
        while (cur_layer != nullptr) {
            Symbol* sym = cur_layer->findInLayer(name);
            if (sym != nullptr) {
                level_diff = current_layer_->getLevel() - cur_layer->getLevel();
                return sym;
            }
            cur_layer = cur_layer->getOuter();
        }
        // 未找到则抛异常
        throw SymbolError(SymErrType::UNDEF, name, current_line_);
    }

    // ========== 作用域管理（适配<proc>的内层/外层切换） ==========
    // 进入过程内层
    void enterProcLayer() {
        total_level_++;
        current_layer_ =  make_unique<SymLayer>(total_level_, current_layer_.release());
    }

    // 退出过程内层（切回外层）
    void exitProcLayer() {
        if (current_layer_->getLevel() == 0) {
            throw  runtime_error("无法退出全局层");
        }

        SymLayer* outer = current_layer_->getOuter();
        current_layer_.reset(outer);  // 切换到外层
        total_level_--;
    }

    // ========== 辅助API ==========
     
    
	// 找到本层最近的没有入口地址的过程符号（用于过程嵌套结束回填）
    string findNearestUnfilledProc() {
        SymLayer* cur_layer = current_layer_.get();
        while (cur_layer != nullptr) {
            Symbol* cur_sym = cur_layer->findInLayer("");
            while (cur_sym != nullptr) {
                if (cur_sym->getType() == SYMBOLTYPE::PROC &&
                    cur_sym->getProcEntryAddr() == -1) {
                    return cur_sym->getName();
                }
                cur_sym = cur_sym->getNext();
            }
            cur_layer = cur_layer->getOuter();
        }
		throw  runtime_error("未找到需要回填的过程符号");
	}

    // 回填过程入口地址，过程嵌套结束回填
    void fillProcEntry(int entry_addr) {
        int level_diff;
		string name = findNearestUnfilledProc();
        Symbol* proc_sym = findGlobal(name, level_diff);
        if (proc_sym->getType() != SYMBOLTYPE::PROC) {
            throw SymbolError(SymErrType::TYPE_MISMATCH, name, current_line_);
        }
        proc_sym->setProcEntryAddr(entry_addr);
    }

    // 检查过程参数个数匹配
    void checkParamCount(const  string& proc_name, int arg_count) {
        int level_diff;
        Symbol* proc_sym = findGlobal(proc_name, level_diff);
        if (proc_sym->getType() != SYMBOLTYPE::PROC) {
            throw SymbolError(SymErrType::TYPE_MISMATCH, proc_name, current_line_);
        }
        if (arg_count != proc_sym->getProcParamCount()) {
            throw SymbolError(SymErrType::PARAM_MISMATCH, proc_name, current_line_);
        }
    }

    // 打印整个符号表（调试）
    void printTable() const {
         cout << "\n===== PL/0 符号表（总层数：" << total_level_ + 1 << "）=====" <<  endl;
        // 遍历所有层（从当前层到全局层）
        SymLayer* cur_layer = current_layer_.get();
         vector<SymLayer*> layers;
        while (cur_layer != nullptr) {
            layers.push_back(cur_layer);
            cur_layer = cur_layer->getOuter();
        }
        // 逆序打印（全局层先打印）
        for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
            (*it)->printLayer();
        }
         cout << "==============================" <<  endl;
    }

    // 行号管理
    void incLine() { current_line_++; }
    int getCurrentLine() const { return current_line_; }

    // 错误标记
    void setErrFlag(bool flag) { err_flag_ = flag; }
    bool hasError() const { return err_flag_; }

private:
     unique_ptr<SymLayer> current_layer_;  // 当前层（智能指针管理生命周期）
    int total_level_;                          // 总嵌套层数
    int current_line_;                         // 当前解析行号
    bool err_flag_;                            // 错误标记
};

