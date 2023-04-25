#ifndef Enum_HPP
#define Enum_HPP

// 判断输入的命令 以 ‘.’ 开始的 是否为 meta-commands
using MetaCommandResult = enum { META_COMMAND_SUCCESS,
                                 META_COMMAND_UNRECOGNIZED_COMMAND
};

// 输入命令的合法性
using PrepareResult = enum { PREPARE_SUCCESS,
                             PREPARE_SYNTAX_ERROR,
                             PREPARE_UNRECOGNIZED_STATEMENT
};

// 支持的两种操作：“插入” 和 “打印所有行”
using StatementType = enum { STATEMENT_INSERT,
                             STATEMENT_SELECT };

// 执行结果类型
using ExecuteResult = enum { EXECUTE_SUCCESS,
                             EXECUTE_TABLE_FULL };

#endif