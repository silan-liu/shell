shell 对于程序员来说，应该是个熟悉得不能再熟悉的老朋友了，几乎天天都会接触。当我们噼里啪啦敲一堆命令后，它会帮我们执行，然后显示结果。

那么 shell 究竟是什么？

简单来说，shell 是一个命令行接口，是一个程序，是用户与操作系统内核交互的桥梁。它的主体功能包括：

- 等待读取命令
- 解析命令
- 执行命令

从下图中，我们很容易观察到的一个现象是：当输入一个命令，回车执行后，程序并没有退出，而是接着等待继续输入。

![](https://cdn.jsdelivr.net/gh/silan-liu/picRepo/img20210304090231.jpg)

所以，我们可以猜想，它是一个不断循环的过程。内部实现一定有个循环，重复的在做上面所述的 3 个步骤。

那么接下来，我们就来具体实战下，如何用 c 来编写一个简单的 shell。请放心阅读，因为真的很简单😃。

## 效果预览

首先来看下效果，以 `$ silan >` 开头的就是我们编写的 shell。如下图所示：

![](https://cdn.jsdelivr.net/gh/silan-liu/picRepo/img20210306093709.png)

## 循环体

整个循环包括读取、解析、执行，是否退出循环根据命令执行的返回值来确定。如下图所示：

![](https://cdn.jsdelivr.net/gh/silan-liu/picRepo/img20210305213929.png)

下面代码就是循环的框架。我们先将其实现空出来，在后面填入。

```objectivec
void lsh_loop()
{
  int status;

  do
  {
    printf("\n$ silan > ");

    // 读取

    // 解析

    // 执行，返回 status

  } while (status);
}
```

在每个循环中，都会预先打印 `$ silan >` 一串提示符，看起来跟我们平常使用的 shell 是不是很类似😆。

另外，还可以自定义添加提示元素，比如显示当前时间。

## 读取命令

这一步，等待用户输入命令，以空格符分割。当键入回车或者 `CTRL+D` 时，代表输入完成。

这里，我们使用 `getline` 来获取输入的字符串，并使用 `feof` 判断是否为结束符。

```objectivec
// 读取命令
char *lsh_read_cmd()
{
  char *cmd = NULL;
  size_t buffSize = 0;

  if (getline(&cmd, &buffSize, stdin) == -1)
  {
    // eof 结束符
    // ctrl+d/回车
    if (feof(stdin))
    {
      exit(EXIT_SUCCESS);
    }
    else
    {
      perror("readline");
      exit(EXIT_SUCCESS);
    }
  }

  return cmd;
}
```

## 解析命令

在获取到输入的命令后，需要对其进行解析，识别出命令和参数。

这里使用 `strtok` 来进行分割，将分割之后的数据放入数组中，数组类型是字符串，`char *`。

由于输入的字符串个数是未知的，所以不能事先给数组确定分配多少空间。因此在处理过程中，会涉及到扩容的操作。

简单说说如何扩容。

首先，默认给数组分配 N 个空位。在处理过程中，不断将字符串放入数组。若数组长度超过了 N，则进行扩容，每次空位增加 N。

比如第一次扩容后的空位为 2 * N，第二次扩容后的空位为 3 * N，以此类推。

下面的实现中，默认定义了 64 个空位。

```objectivec
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

// 解析命令
char **lsh_parse_cmd(char *cmd)
{
  int buffSize = LSH_TOK_BUFSIZE;
  int position = 0;

  char **tokens = malloc(buffSize * sizeof(char *));
  char *token;

  if (!tokens)
  {
    fprintf(stderr, "lsh: alloction error!\n");
    exit(EXIT_FAILURE);
  }

  // 分隔字符串
  token = strtok(cmd, LSH_TOK_DELIM);
  while (token != NULL)
  {
    tokens[position++] = token;

		// 扩容处理
    if (position >= buffSize)
    {
      // realloc
      buffSize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, buffSize * sizeof(char *));

      if (!tokens)
      {
        fprintf(stderr, "lsh: alloction error!\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }

  // 结尾空字符
  tokens[position] = NULL;

  return tokens;
}
```

## 执行命令

当解析出命令后，就需要执行命令，进行相应操作。

这里启用一个子进程来进行处理，使用 `fork` 创建子进程。

由于 `fork` 出来的子进程与父进程是执行同样的代码，而我们需要执行解析出来的命令，因此使用 `execvp` 来替换掉子进程的程序。

> `execvp` 是 `exec` 的变体，p 表示命令不需要完整路径，v 表示参数以数组方式传入。

最后，父进程等待子进程完成。

```objectivec
// 启动子进程，执行命令
int lsh_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    // child
    if (execvp(args[0], args) < 0)
    {
      perror("lsh exec cmd error!");
    }

    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    perror("lsh fork error!");
  }
  else
  {
		// 等待子进程
    wpid = waitpid(pid, &status, WUNTRACED);
  }

  return 1;
}
```

## 内置命令

虽然我们可以使用 `execvp` 来执行新的程序，但是有些命令还是不能在子进程中实现。

- 比如 cd，用于切换目录。目录是跟进程相关联的，假若在子进程中切换，那么它只是将子进程的目录进行了更改，但父进程仍未受到影响。
- 类似的还有 exit，退出程序。在子进程中实现的话，也仅仅是子进程自身的退出。

所以，还需要额外处理一些命令，我们称之为内置命令。

这里，我们添加三个内置命令，cd、exit、help。

- cd，用于切换目录
- exit，退出程序
- help，打印内置命令信息

### 内置命令声明

内置命令字符串定义如下：

```objectivec
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};
```

内置命令函数声明如下：

```objectivec
int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};
```

内置命令个数计算：

```objectivec
int lsh_num_builtions()
{
  return sizeof(builtin_str) / sizeof(char *);
}
```

### cd

cd 的实现比较简单，使用 `chdir` 就好，下面的代码中大部分是错误处理。

这里的 `args[0]` 是 `"cd"`，`args[1]` 是路径参数。

```objectivec
int lsh_cd(char **args)
{
  if (args == NULL)
  {
    perror("cd args error!");
  }

  if (args[1] == NULL)
  {
    fprintf(stderr, "lsh: no args\n");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("chdir error!");
    }
  }
  return 1;
}
```

### exit

退出程序，只需在函数实现中返回 0 即可。此时循环会中断，然后程序自然退出。

```objectivec
int lsh_exit(char **args)
{
  return 0;
}
```

### help

打印内置命令信息。

```objectivec
int lsh_help(char **args)
{
  printf("lsh builtin cmd:\n");

  for (int i = 0; i < lsh_num_builtions(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}
```

### 额外处理

添加内置命令后，在执行命令时，就首先需要判断一下命令是否是内置的。若是，则优先走内置实现；否则，使用启动子进程的方式处理。

最后，执行命令的流程变为：

```objectivec
// 执行命令
int lsh_execute_cmd(char **args)
{
  if (args == NULL || args[0] == NULL)
  {
    return -1;
  }

  char *cmd = args[0];

	// 判断是否是内置命令
  for (int i = 0; i < lsh_num_builtions(); i++)
  {
    char *builtin_cmd = builtin_str[i];
    if (strcmp(cmd, builtin_cmd) == 0)
    {
			// 执行自定义的函数
      return (*builtin_func[i])(args);
    }
  }

	// 启动子进程
  return lsh_launch(args);
}
```

就这样，一个简单的 shell 就完成了。

## 总结

这篇文章主要介绍了 shell 是什么，以及如何动手实现 shell 的主体功能，包括等待输入命令、解析命令、执行命令、内置命令等。

整体实现起来还是比较简单的，希望对你有所帮助🤩~

## 参考资料

- [https://brennan.io/2015/01/16/write-a-shell-in-c/](https://brennan.io/2015/01/16/write-a-shell-in-c/)

