`make fat`会重新编译user/下的文件，并将可执行文件*.b拷贝到fs.img的根目录。

若仅仅通过链接进内核来执行可执行程序(CREATE_PROCESS_PRIORITY)，则不需要重新执行`make fat`

