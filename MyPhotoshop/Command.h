#pragma once
#include <functional>

class CCommand
{
public:
    virtual ~CCommand() {}
    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

// 通用命令类
class CGenericCommand : public CCommand
{
private:
    std::function<void()> m_executeFunc; // 执行操作的函数
    std::function<void()> m_undoFunc;    // 撤回操作的函数

public:
    CGenericCommand(std::function<void()> executeFunc, std::function<void()> undoFunc)
        : m_executeFunc(executeFunc), m_undoFunc(undoFunc) {}

    void Execute() override
    {
        if (m_executeFunc)
            m_executeFunc();
    }

    void Undo() override
    {
        if (m_undoFunc)
            m_undoFunc();
    }
}; 
#pragma once
