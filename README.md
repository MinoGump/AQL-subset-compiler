# AQL子集编译器

AQL（Annotation Query Language）是用于文本分析的语言，可以从非结构化或半结构化的文本中提取结构化信息的语言。比如在一篇文章中提取一个或多个关键信息，在文章中查找指定信息串。

### AQL语法

---

类似SQL，AQL也有查询语言类似的语法。View类似数据库中的Table，而每个View中的Column类似Table中的列。

- 按正则表达式查询

其中正则表达式写于两个‘/’之间，正则表达式语法请见[连接](https://msdn.microsoft.com/zh-cn/library/ae5bf541(v=vs.90).aspx)，其中Document是默认的View，text是Document的属性列，用于存储文章的文本。

```
create view Cap as
  extract regex /[A-Z][a-z]*/
    on D.text as Cap
  from Document D;
```

另外，正则表达式也可以包含组。一个组是用括号括起来的内容，从左至右的n个括号依次为从1到n个组，每个组可以用return语句返回。并且 group 0 指整个匹配的内容。

```
create view Cap as
  extract regex /([A-Z])([a-z]*)/
    on D.text
  return group 0 as Cap
     and group 1 as Uppercase
     and group 2 as Lowercase
  from Document D;
```

- 按模式查询

根据之前匹配过的所有模式（pattern），我们可以再次进行二次匹配。如利用匹配过的Per和Loc两个模式，再次匹配PerLoc新模式。

其中Token指文章中任意以字母或者数字组成的无符号分割的字符串，或者单纯的特殊符号，不包含空白符（blank）。
‘{’ 和 ‘}’之间包含的两个数字代表最小值n和最大值m，表示前面的一个Token或者模式（pattern）有最少n个最多m个在文章中相连。

```
create view PerLoc as
  extract pattern (<P.Per>) <Token>{1,2} (<L.Loc>)
  return group 0 as PerLoc
    and group 1 as Per
    and group 2 as Loc![](/home/mino/Pictures/Screenshot from 2015-12-30 23:47:57.png) 
  from Per P, Loc L;
```

另外，一个组（group）也可以包含任意多的模式或者token或者正则表达式。下面的例子表示第一组包含一个模式和一个正则表达式

```
create view Loc as
  extract pattern (<C.Cap> /,/) (<S.Stt>)
  return group 0 as Loc
    and group 1 as Cap
    and group 2 as Stt
  from Cap C, Stt S;
```

- 选择

利用之前匹配到的View的列，我们可以通过选择操作快速获得对应的列。

```
create view PerLocOnly as
  select PL.PerLoc as PerLoc
  from PerLoc PL;
```

### 编译与运行

---

程序的目录结构如下：

![](file:///home/mino/Pictures/4.png)

其中dataset目录是用于放置文章和AQL文件的，AQL文件置于dataset目录下，文章在dataset目录下新建一个对应的文件夹存放

**编译**

进入src文件夹，输入make命令编译，在根文件夹中生成AQL可执行文件。

**运行**

- 分析单一文件

```sh
./AQL    <sample.aql>    <sqmple.input>
```

- 分析文件夹下的所有文件

```sh
./AQL    <sample.aql>    <sqmple_folder>
```

---


