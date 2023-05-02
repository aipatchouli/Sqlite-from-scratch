### 一个简单的数据库（以Sqlite为目标）     
+ 前端（解析器 parser：SQL查询语句 --> 数据库虚拟机字节码）
    + tokenizer (文本被拆分为“标记”流)    
    + syntax tree
        + 解释器 naive interpreter
    + compiler

+ 后端（数据库虚拟机字节码 --> 数据库操作）
    + virtual machine     
         + 操作表或索引（B tree）（VM 本质上是一个关于字节码指令类型的大 switch 语句）
    + B tree     
         + 暂时使用数组
         + 逻辑结构
    + pager     
         + 文件读写的具体位置（磁盘，内存）
    + os interface
         + 持久化 // POSIX标准: open,read,write,lseek,close