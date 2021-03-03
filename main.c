#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

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
    printf("token:%s\n", token);

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
    // why do while?
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status));
  }

  return 1;
}

// builtin function
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

// builtin commands
char *builtin_str[] = {
    "cd",
    "help",
    "exit"};

int (*builtin_func[])(char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit};

int lsh_num_builtions()
{
  return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "lsh: no args");
  }
  else
  {
    if (chdir(args[1]) != 0)
    {
      perror("chdir error!");
    }
  }
  return 0;
}

int lsh_help(char **args)
{
  printf("lsh builtin cmd:");

  for (int i = 0; i < lsh_num_builtions(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }

  return 0;
}

int lsh_exit(char **args)
{
  return 0;
}

// 执行命令
int lsh_execute_cmd(char **args)
{
  return lsh_launch(args);
}

// read - parse - execute
void lsh_loop()
{
  char *cmd;
  char **args;
  int status;

  do
  {
    printf("silan> ");

    // 读取
    cmd = lsh_read_cmd();
    printf("cmd:%s\n", cmd);

    // 解析
    args = lsh_parse_cmd(cmd);
    printf("args:%p\n", args);

    // 执行
    status = lsh_execute_cmd(args);

    free(cmd);
    free(args);
  } while (status);
}

int main()
{
  lsh_loop();
  return 0;
}