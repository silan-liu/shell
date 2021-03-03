#include <stdio.h>
#include <stdlib.h>

// 读取命令
char *lsh_read_cmd()
{
  return NULL;
}

// 解析命令
char **lsh_parse_cmd(char *cmd)
{
  return NULL;
}

// 执行命令
int lsh_execute_cmd(char **args)
{
  return 0;
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

    // 解析
    args = lsh_parse_cmd(cmd);

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