# Mondis
Mondis is a key-value database powered by redis and addToSlot some new feature。
# 什么是Mondis
Mondis是一个key-value数据库，它很像redis，但是支持许多redis不支持的新特性。实际上，它的名字mondis就是取自mongodb与redis。下面，让我们了解以及学习如何来使用它。
# 选择mondis的理由
## mondis嵌套
mondis支持数据结构的任意嵌套，mondis键的取值只能是字符串，但是值的取值可以是string,list,set,zset,hash的任意一种。mondis嵌套意味着list,zset的元素与hash类型键值对的值可以是另一个list,set,zset或者hash，就像json那样。注意，set类型的元素只能是string，这是因为set底层采用hash表进行实现，而list,zset与hash没有默认的哈希函数。mondis支持任意复杂，任意深度的嵌套。
## json支持
mondis查询类似于redis查询，但是不同的是mondis查询返回的格式是json。这使得mondis更方便使用。而且mondis支持json持久化与恢复。
## 自动转化数据结构
mondis可以把json格式的value自动转化为对应的数据结构。例如，"this is a test"将会被解析成一个字符串类型的value，内容是this is a test。"{}"将会被解析成没有键值对的hash。"[]"将会被解析成没有元素的list,但是set和zset同样对应json中的list，这时候就要采用一种区分方式，mondis采用的是根据地一个元素区别，默认是list，如果第一个元素是"LIST"，那么就会被解析为list，set和zset同理。"[{“this is a test”}]"则会被解析成有一个hash元素的list，这个list里面有一个元素，这个元素是字符串this is a test。自动转化的数据结构支持任意形式和深度的嵌套，就像在第一条中所说的那样。
## mondis多态命令
在redis里面，我们在操作的时候必须指定底层数据的类型，比如list命令全部以l开头，zset命令全部以z开头，hash命令全部以hash开头，
但是在mondis里面，这些统统不需要。mondis命令具有多态性，相同的命令在不同数据结构上的效果是不同的，只需要执行命令而无需关心底层数据结构是
什么。如果执行了不合适的命令，mondis会处理这种情况，不用担心崩溃。
## mondis定位命令
由于mondis支持任意嵌套，有时候我们要操作一个数据对象的一部分，它的嵌套层数很深，此时就需要用到定位命令。定位命令是locate，它的作用是定位到你要操作的部分上面。locate命令可以连续使用，每一条locate命令递进一层，locate命令之间用|隔开。假如目前键空间里面有一个list，它的key是test，你要对这个list的第一个元素执行某些操作，需要这样写：locate test|locate 0|你要执行的命令。
## mondis持久化
mondis支持两种持久化方式，json与aof。json类似于rdb，但是持久化的格式是json，这使得持久化文件方便迁移并解析。aof持久化则与redis的aof完全相同，除了命令格式。但是目前mondis还不支持aof重写。
## 跨平台
Mondis在设计的时候已经考虑到了平台兼容性的问题，mondis是跨平台的，在windows以及unix系统上都能正常使用。不过要在各自的平台上编译二进制文件。
## 事务支持回滚
Mondis的事务支持回滚，就像传统关系型数据库那样。这意味着如果在事务执行过程中某条命令没有执行成功，已经执行的命令就会回滚，就像从来没有执行过这条命令一样。然后事务的执行状态将会被重置。
## 集群raft支持
mondis的主从复制集群采用了raft算法实现最终一致性，这意味着如果leader节点在任意时刻挂掉了，集群都不会因此受到影响。raft算法会处理好所有的一切，包括重新选leader，以及同步到其他所有节点。
## 写命令重定向
mondis slave节点可以自动将收到的写命令转发到master节点执行，这个行为对客户端是完全透明的。这意味着如果a,b,c组成了一个集群，a是master,b,c是slave，那么b,c收到的写命令将会自动转发给a执行，b,c的客户端对此毫不知情，就好像这条命令是由b,c执行的一样。此功能需要在配置文件中开启，如果没开启，那么slave节点在收到写命令后拒绝执行并返回一个错误。
## mondis底层实现相对于redis的改进
### list
在mondis里面，list还是用链表实现。不同的是mondis保存了对象指针与索引的双向映射，这样虽然多占用了一些空间，不过支持元素的随机访问。list同时支持redis里面的list所有操作。这些操作的均摊时间复杂度均为常数。
### set
mondis的set采用的是value为空的hash表进行实现，听起来似乎与redis没什么不同，但是mondis的hashmap采用avl树解决哈希冲突，减小了哈希表操作的常数因子。
### zset
redis的zset采用跳表+哈希表实现，编码异常复杂，空间占用也没很大优势。mondis的zset采用伸展树实现，在时间与空间复杂度上进行了很好的权衡，而且伸展树方便的支持区间操作。
### hash
redis的hash在键值对数量不多时采用ziplist，虽然空间占用小，但是查询时必须线性扫描，而且插入时有可能连锁更新影响性能。在键值对数量较多时采用哈希表，浪费大量空间。mondis的hash采用平衡树实现，在时间与空间复杂度上进行了很好的权衡。
# 开始使用Mondis
使用Mondis非常简单，首先需要取得Mondis的可执行文件，然后在终端输入./mondis xxx.conf。这时候可以看到终端上打印出启动信息，表示Mondis启动成功。xxx.conf即配置文件路径，如果不指定配置文件，Mondis将以默认配置启动。
Mondis启动后并不能立即使用。实际上，你需要登录才能执行命令。登录的命令是login username password。username与password可以在配置文件中指定，如果没有配置文件，默认的username为root，password为admin。其他命令将在下面介绍。
# mondis的数据结构
mondis支持redis所支持的全部数据结构，除此之外，mondis还支持二进制数据。也就是说Mondis支持string,list,set,zset,hash,binary
六种数据结构。下面一一进行介绍。
## string
顾名思义，string就是字符串，它跟redis的string，以及其他编程语言中的字符串并没有任何不同。但是string有两种编码方式,RAW_STRING和RAW_INT.
RAW_STRING就是字符串编码，如果string可以被转化为一个int的话，那么它将采用RAW_INT编码，这意味着在内存中它将是一个int。序列化结果是一个字符串。
## list
即列表，相当于java里面的LinkedList，但是支持随机访问，时间复杂度为常数。除此之外还支持双端队列操作，即push_front,push_bakc,pop_front和pop_back。list里面的元素可以是任意类型。序列化结果是一个json数组，内部元素按照list内部顺序排列。但是第一个元素是一个"LIST"字符串，表明这是一个LIST序列化的结果，用于反序列化时参考。
## set
即集合，集合中不包含重复的元素，而且集合中的元素是无序的。集合支持add,remove,size和exists操作，时间复杂度均为常数。与list不同，集合中的元素只能是string或者binary。序列化结果是一个json数组，内部元素顺序未定义。但是第一个元素是一个"SET"字符串，表明这是一个SET序列化的结果，用于反序列化时参考。
## zset
即有序集合，在存取元素时需要指定一个int的score，内部元素根据score从小到大有序。但是score不能指定int类型的极值。zset里面的元素可以是任意类型，支持set的所有操作，同时支持根据rank的区间访问，区间删除和根据score的区间访问，区间删除，还有更改一个元素的score。序列化结果是一个json数组，元素是内部元素，顺序按score从大到小排列。但是第一个元素是一个"ZSET"字符串，表明这是一个ZSET序列化的结果，用于反序列化时参考。zset的序列化把四个字节的score转化为一个四字节大小的byte数组放在对应的元素前面。
## hash
即键值对的集合，key只能是RAW_STRING编码的string，值可以是任意类型。支持add,remove,size和exists操作。序列化结果是一个json对象，键值对即hash的键值对。
# Mondis命令
## 概述
mondis有很多命令，分为键空间命令，控制命令，locate命令与数据对象命令。键空间命令即直接在数据库空间执行的命令，
locate命令即定位命令，用来定位要操作的数据对象。数据对象命令即操作对应数据对象的命令。mondis命令有若干参数，
参数分为普通参数与string参数，普通参数就是一段连续的token，中间不能有空格，两端没有分号，如abc,1235等。
字符串参数就是一对引号括起来的参数，中间可以有任意字符。Mondis命令具有多态性，也就是说在操作数据对象时并不需要
关心这个数据对象的类型，只要它可以执行这条命令，命令就会被正确执行。否则你将会得到一个错误。mondis命令不区分大小写，
这意味着LOGIN，login与LoGIn均能被正确识别。下文以&lt;&gt;指定普通参数，以[]指string参数。注意，string参数
在被读取时是忽略两端的引号的，这意味着字符串参数"abc"将会被读取为abc。
## 键空间命令
### set &lt;key&gt; [content]
set命令即添加一条键值对。第一个参数是键，第二个是值。如果键值对已存在，则值将会被覆盖。第二个参数应当是标准的
json字符串，它将会被自动解析成合适的数据结构。
### del &lt;key&gt;
删除对应的key。
### get &lt;key&gt;
返回对应key的json表示
### exists &lt;key&gt;
检查是否存在对应的key。
### renameKey &lt;oldkey&gt; &lt;newkey&gt;
修改oldkey为newkey。
### type &lt;key&gt;
返回key对应的value底层编码类型。类型有RAW_STRING,RAW_INT,RAW_BIN,LIST,SET,ZSET,HASH
### move &lt;new&gt;
将一对kv迁移到编号为new的键空间中。
### size
返回键空间内键的数量

## 控制命令
### selectDb &lt;db&gt;
修改当前键空间为编号为db的键空间。
### save &lt;filepath&gt;
将整个当前键空间以json持久化到file里面。save命令的默认实现是开启子进程持久化，因此不需要担心性能问题。
mondis不提供bgsave。
### save_all &lt;filepath&gt;
将所有键空间以json持久化到file里面。基本与save相同。
### login &lt;username&gt; &lt;password&gt;
以username和password登录
### exit
退出登录并退出Mondis server。
### disconnect_client
断开与发送该命令的客户端的连接
### disconnect_master
断开当前服务器与主服务器的连接，仅当当前服务器是从服务器时有效。
### undo
撤销所执行的上一条写命令。
### multi
为当前客户端开启一个事务。同时最多只能存在一个事务。
### exec
执行当前的事务并且退出事务
### discard
抛弃当前未执行事务的所有命令并关闭事务。
### watch &lt;key&gt;
监视当前键空间的为key的键，如果事务执行前该键对应的value发生了变化则事务执行失败并自动回滚。
### unwatch &lt;key&gt;
取消监视当前键空间为key的键。

## string命令
string命令分为两大类，有些只能在RAW_INT上执行，有些可以在RAW_STRING上执行。下面分别介绍
## RAW_STRING命令
### get &lt;position&gt; 
返回position处的字符。
### set &lt;position&gt; [char]
设置position处的字符为char。char的长度必须为1。
### get_range &lt;start&gt; &lt;end&gt;
获得从start到end的子串，包括start处的字符不包括end。
###set_range &lt;start&gt; [new]
将从start开始长度等于new的子串设为new。从new的第一个字符开始设置,如果new的长度大于从start开始到
结尾的子串，new多余的部分会被截断。
### set_range &lt;start&gt; &lt;end&gt; [new]
将从start到end的子串设为new。从new的第一个字符开始设置，多余的部分会被截断。
### strlen
返回该字符串的长度
### insert &lt;location&gt; [new]
将new插入到原来字符串第location个字符之前。
##RAW_INT命令
### incr
值加一。
### decr
值减一。
### incr_by &lt;increment&gt;
值加上增量。
### decr_by &lt;decrement&gt;
值减去增量。
### to_string
将底层编码变成RAW_STRING

## list命令
### set &lt;index&gt; [data]
将list的第index个元素设为data。
### get &lt;index&gt;
获取list的第index个元素
### size
返回list中元素数量
### front
获取第一个元素，不弹出
### back
获取最后一个元素，不弹出
### push_front [data]
将data添加到list开头
### push_back [data]
将data添加到list末尾
### pop_front
从list开头弹出并返回这个元素。如果list为空，则返回一个错误。
### pop_back 
从list末尾弹出并返回这个元素。如果list为空，则返回一个错误。
### get_range &lt;from&gt; &lt;to&gt;
获取并返回list下标从from到to之间的所有元素。包括from处的元素，不包括to。返回的格式是json数组。

## set命令
### addToSlot [data]
将data添加到list里面。data的反序列化结果的编码只能是RAW_INT或者RAW_STRING，否则会出错。下同。
### remove [data]
从set里面删除data。
### size
返回set中元素数量
### exists [data]
检查set中是否存在data

## zset命令
### get_by_score &lt;score&gt;
获得zset里面分数等于score的元素
### get_by_rank &lt;rank&gt;
获得zset里面排名等于rank的元素
### remove_by_score &lt;score&gt;
删去zset里面分数为score的元素
### remove_by_rank &lt;rank&gt;
删去排名为rank的元素。
### get_range_by_score &lt;from&gt; &lt;to&gt;
获得分数在from与to之间的所有元素。包括from，不包括to
### get_range_by_rank &lt;from&gt; &lt;to&gt;
获得排名在from与to之间的所有元素。包括from，不包括to
### remove_range_by_score &lt;from&gt; &lt;to&gt;
删除分数在from与to之间的所有元素。包括from，不包括to
### remove_range_by_rank &lt;from&gt; &lt;to&gt;
删除排名在from与to之间的所有元素。包括from，不包括to
### set &lt;score&gt; [data]
添加一个分数为score的元素进zset。如果该score已存在，则覆盖。
### exists &lt;score&gt;
检查是否存在分数为score的元素。
### size
返回zset中元素数量
### change &lt;old&gt; &lt;new&gt;
将分数为old的元素的分数变为new。如果分数为old的元素不存在，或者分数为new的元素已存在，则出错。
### score_to_rank &lt;score&gt;
获得分数为score的元素对应的名次，如果没有这样的元素，则出错。
### rank_to_score &lt;score&gt;
获得排名为rank的元素的分数。

## hash命令
### set &lt;key&gt; [value]
添加一个键值对。如果key已存在，则覆盖。
### remove &lt;key&gt;
删除一个键值对。
### get &lt;key&gt;
返回key所对应的value。
### exists &lt;key&gt;
检查是否存在key。
### size
返回键值对的数量。

## locate命令
locate命令用于定位要操作的数据对象。下面是一个简单的示例。
执行set a "{"test"}"以后，a就是一个有一个元素的list，这个元素是"test"。那么我们要操作test，就要定位到
这个元素。首先在键空间中定位a，然后在a中定位"test"。假如我们要获得test的第一个字符，那么应该执行如下命令：
locate a|locate 1|get 1。locate a在键空间中定位到a，locate 1定位到a的第一个元素，然后获得test的第一个字符。
竖线的作用是分隔locate命令。相邻的locate命令之间必须有竖线。下面详细介绍各个数据结构的locate。
### string,binary，set
string，binary与set无法locate，如果尝试执行，将会出现一个错误。
### list
list的locate命令格式是locate &lt;index&gt;。
### zset
zset的locate命令格式是locate "RANK" &lt;rank&gt;或者locate "SCORE" &lt;score&gt;
### hash
hash的locate命令格式是locate &lt;key&gt;。
### 键空间
与hash相同。

# Mondis配置文件
Mondis配置文件是.conf格式的文本文件，配置项格式为&lt;key&gt;=&lt;value&gt;，&lt;key&gt;必须在每一行的开头，&lt;value&gt;必须在每一行
的末尾。一个配置项一行，等号前后不能有空格。每一行注释都必须以#开头。server启动的唯一参数就是配置文件路径，如果未指定配置文件，
将以默认配置进行启动。下面依次说明每一个配置项的作用。

## daemonize=&lt;true|false&gt;
是否以守护进程方式运行Mondis server，默认为false。在windows上此选项将被忽略。

## port=&lt;integer&gt;
mondis server的监听端口号。默认为2379。

## username=&lt;string&gt;
mondis server的登录用户名

## password=&lt;string&gt;
mondis server的登录密码

## databaseNum=&lt;integer&gt;
mondis server的数据库（即键空间）数量，默认为16。

## databaseId=&lt;integer&gt;
mondis server启动以后的默认数据库id。从0到databaseNum-1。

## aof=&lt;true|false&gt;
是否开启aof持久化。

## aofSyncStrategy=&lt;everySecond|forceSync|osDefault&gt;
aof持久化同步策略。everySecond为每秒同步，也是默认的同步策略。forceSync为每条写命令强制同步。osDefault完全取决于操作系统。

## aofFile=&lt;string&gt;
aof持久化文件路径。

## json=&lt;true|false&gt;
是否开启json持久化。

## jsonFile=&lt;string&gt;
json持久化文件路径。

## recovery=&lt;aof|json|neither&gt;
启动时是否从aof或者json文件中恢复。neither表示不恢复，为默认选项。

## workDir=&lt;string&gt;
工作目录，默认文件均会在此目录下寻找，找不到则自动创建。

## beSlaveOf=&lt;true|false&gt;
启动时是否同步主服务器，默认为false

## masterIP=&lt;string&gt;
主服务器ip。

## masterPort=&lt;integer&gt;
主服务器端口号

## masterUsername=&lt;string&gt;
主服务器用户名

## masterPassword=&lt;string&gt;
主服务器密码

## logFile=&lt;string&gt;
日志文件路径

## maxClientNum=&lt;integer&gt;
最大并发客户端数,默认为1024

## maxSlaveNum=&lt;string&gt;
最大从服务器数，默认为1024.

## maxCommandReplicaBufferSize=&lt;integer&gt;
复制积压命令缓冲区大小，可以理解为最多保存执行过的写命令的条数。默认为1024*1024,即1048576。此选项不宜设的过小，
否则在主同步时大概率会进行完全同步影响效率。

## maxCommandPropagateBufferSize=&lt;integer&gt;
命令传播缓冲区大小，缓存主服务器已经执行完毕，需要传播给从服务器的写命令。默认为1024

## autoMoveCommandToMaster=&lt;true|false&gt;
当前服务器为从服务器时，是否自动将客户端的写命令转发到主服务器，如果为false将丢弃这条命令并返回一个错误。默认为true。

## forbidOtherModifyInTransaction=&lt;true|false&gt;
当事务a监控了键key，事务b修改key时是否阻止。如果开启此选项，事务b的修改将会被丢弃并返回一个错误。默认为false。
# 写命令清单
## BIND
## DEL
## RENAME
## SET_RANGE
## REMOVE_RANGE
## INCR
## DECR
## INCR_BY
## DECR_BY
## INSERT
## PUSH_FRONT
## PUSH_BACK
## POP_FRONT
## POP_BACK
## ADD
## REMOVE
## REMOVE_BY_RANK
## REMOVE_BY_SCORE
## REMOVE_RANGE_BY_RANK
## REMOVE_RANGE_BY_SCORE
## WRITE
## TO_STRING
## TO_INTEGER
## CHANGE_SCORE
