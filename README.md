# CodeMerger

一个用于把大型项目目录下的所有源码/文本文件**合并成单个 Markdown 文件**的 Windows 小工具，
方便用一次阅读代替在 IDE 里来回翻文件，也便于做代码审查、交给 AI 做上下文理解、或归档留存。

工具基于 MFC（VS2015，x64），提供图形界面，支持拖拽添加目录、多目录递归扫描、自动编码识别
与统一转 UTF-8，并额外提供一个**默认关闭的原地转码功能**，用于把老项目批量升级为 UTF-8 格式。

---

## 功能特性

### 合并

- **多目录批量合并**：支持同时添加多个目录，递归扫描子目录，全部输出到一个 Markdown 文件。
- **智能后缀过滤**：默认内置一份常用的源码/文本后缀白名单（`.c` `.cpp` `.h` `.hpp` `.cs` `.py` 等三十余种），也可在 INI 中自定义。
- **自动编码检测与转换**：
  - UTF-8（带 / 不带 BOM）
  - UTF-16 LE / BE（带 BOM）
  - UTF-16 LE / BE（无 BOM，启发式识别）
  - ANSI / GBK（中文 Windows 默认代码页）
  - 输出统一为 **UTF-8（无 BOM）** 的 Markdown，跨平台阅读无障碍。
- **单文件大小限制**：默认 2 MB，超过的文件会被跳过并在输出中标注，避免误吞二进制/大文件。
- **进度反馈**：任务栏进度条 + 耗时统计。
- **拖拽支持**：目录可直接拖入主窗口列表。
- **命令行输入**：支持把目录拖到 exe 图标或快捷方式上自动加载。
- **配置持久化**：输入目录、输出目录、窗口尺寸、后缀白名单、单文件大小上限等通过 INI 保存，下次启动自动恢复。

### 原地转码（高风险，默认关闭）

- **用途**：把老项目里的 GBK / ANSI / UTF-16 源码文件**原地转成 UTF-8（可选带 BOM）**，
  一次性完成编码统一，供现代 IDE / 编辑器使用。
- **默认关闭**，UI 不提供入口，只能通过 INI 开启。
- **强制备份**：每个被转换的文件都会先复制为 `xxx.bak`；若 `.bak` 已存在，
  依次尝试 `.bak.1` ~ `.bak.99`，全部占满则跳过并报告。
- **Dry-Run 模式**：可只扫描不修改，生成报告供核对。
- **生成报告**：输出目录下生成 `InPlaceConvertReport.md`，并自动用系统默认程序打开。
- **风险提示**：执行前会弹出强制警告，要求用户自行确认已备份项目。

---

## 使用方法

### 合并模式

1. 下载或自行编译 `CodeMerger.exe`。
2. 双击运行。
3. 点击 **添加目录**，或将一个/多个目录直接拖入列表区。
4. 在 **输出目录** 中选择生成的 `codeAll.md` 存放位置（默认与上次相同）。
5. 点击 **确定**，等待进度条完成。
6. 打开输出目录下的 `codeAll.md`，即可在单个文件中浏览所有源码。

命令行方式（可选）：

```bat
CodeMerger "D:\path\to\project"
```

把目录直接拖到 exe 上也可以触发相同行为。

### 原地转码模式

1. 关闭程序，编辑与 exe 同名的 INI 文件，设置：

   ```ini
   [INI_PRESUFFIX]
   EnableInPlaceConvert=1
   InPlaceExtensions=.c,.cpp,.h,.hpp,.cs
   InPlaceUtf8Bom=1
   InPlaceDryRun=1
   ```

2. **首次强烈建议 `InPlaceDryRun=1`**，先跑一遍看报告，确认哪些文件会被转换。
3. 确认无误后把 `InPlaceDryRun=0`，重新启动程序，选择**输出目录**，点击 **确定**。
4. 弹窗会再次确认；确认后开始原地转码，结束后自动打开 `InPlaceConvertReport.md`。
5. 合并流程会紧接着执行，把转码后的内容汇总到 `codeAll.md`。

> ⚠️ **原地转码前请务必使用 Git / SVN 提交一次，或整体复制一份项目副本。**
> 工具会尽力保护原始文件（备份 + 临时文件 + 原子替换），但不做跨文件事务回滚。

---

## 配置说明（INI）

程序首次运行会在**与 exe 同目录**生成一份 INI 文件（文件名与 exe 同名，后缀 `.ini`）。
示例内容如下：

```ini
[INI_PRESUFFIX]
INI_REMPATH=1
INI_DST_DIRS=D:\output
INI_ALL_ITEMS=D:\project1|D:\project2
Extensions=.txt,.md,.h,.hpp,.c,.cpp,.cs,.py,.cc,.cxx,.inl,.js,.ts,.java,.go,.rs,.json,.xml,.yml,.yaml,.html,.htm,.css,.sh,.bat,.ps1,.sql,.ini,.cfg,.log,.csv
MaxFileSizeMB=2

EnableInPlaceConvert=0
InPlaceExtensions=.c,.cpp,.h,.hpp,.cs
InPlaceUtf8Bom=1
InPlaceDryRun=0

INI_WIN_WIDTH=1024
INI_WIN_HEIGHT=768
```

### 合并相关

| 键 | 含义 |
|---|---|
| `INI_REMPATH` | 启动时是否恢复上次的目录列表与输出目录（`1` 是，`0` 否） |
| `INI_DST_DIRS` | 上次使用的输出目录 |
| `INI_ALL_ITEMS` | 上次添加的输入目录列表，用 `|` 分隔 |
| `Extensions` | 参与合并的后缀白名单，逗号或分号分隔，大小写不敏感，可省略前导点 |
| `MaxFileSizeMB` | 单个文件大小上限（MB），超过则跳过 |
| `INI_WIN_WIDTH` / `INI_WIN_HEIGHT` | 上次窗口尺寸 |

### 原地转码相关

| 键 | 含义 |
|---|---|
| `EnableInPlaceConvert` | 是否开启原地转码（`0` 关闭，`1` 开启），默认 `0` |
| `InPlaceExtensions` | 参与原地转码的后缀白名单，格式同 `Extensions` |
| `InPlaceUtf8Bom` | 目标编码是否带 BOM（`1` 带，`0` 不带），默认 `1` |
| `InPlaceDryRun` | Dry-Run 模式（`1` 只扫描并写报告，不改文件），默认 `0` |

> 修改配置后重新启动程序生效。

---

## 输出格式

### codeAll.md

生成的 `codeAll.md` 结构如下：

```markdown
# 文件清单

# 文件：D:\project\src\main.cpp

> 编码：UTF-8，大小：1234 字节

----------------------------------------

<main.cpp 的完整内容>

----------------------------------------

# 文件：D:\project\src\util.h

> 编码：ANSI/GBK，大小：567 字节

----------------------------------------

<util.h 的完整内容>

----------------------------------------
```

- 每个文件都以 `# 文件：<完整路径>` 作为一级标题。
- 第二行引用块标出**检测到的原编码**和**原始字节数**，方便你判断是否需要手动核查。
- 内容原样保留，不做任何格式改写（不重排换行、不去空白）。
- `codeAll.md` 自身与 `InPlaceConvertReport.md` 都会被自动排除，不会自我嵌套。

### InPlaceConvertReport.md

原地转码后生成的报告，包含：

- 头部：模式（DryRun / 实际执行）、目标是否带 BOM、参与的后缀白名单、开始时间。
- 每个文件的处理结果：`[OK]` / `[SKIP]` / `[DRY-RUN]` / `[FAIL]`，附原编码、目标编码、大小变化、备份文件名。
- 尾部：结束时间、汇总计数、失败文件清单（便于快速定位）。

---

## 支持的编码

| 编码 | 检测方式 | 输出 |
|---|---|---|
| UTF-8（带 BOM） | `EF BB BF` 前缀 | 去 BOM 后原样输出 |
| UTF-8（无 BOM） | 严格字节序列校验（含过长编码、代理区、超范围检查） | 原样输出 |
| UTF-16 LE（带 BOM） | `FF FE` 前缀 | 转 UTF-8 |
| UTF-16 BE（带 BOM） | `FE FF` 前缀 | 转 UTF-8 |
| UTF-16 LE（无 BOM） | 零字节分布启发式判断（偶数长度、0x00 占比 ≥ 20%、奇数位 0x00 比例 ≥ 90% 等） | 转 UTF-8 |
| UTF-16 BE（无 BOM） | 同上，偶数位 0x00 比例 ≥ 90% | 转 UTF-8 |
| ANSI / GBK | 上述均失败时按系统默认代码页（CP_ACP） | 转 UTF-8 |
| 未知编码 | 上述均失败时 | 原文输出，标题标注 `[未知编码]` |

---

## 编译

环境要求：

- Visual Studio 2015（或更高版本）
- MFC 支持
- 目标平台：**x64**（建议）
- 字符集：**Unicode**

步骤：

1. 用 VS2015 打开 `DllTestor.sln`。
2. 平台选择 `x64`，配置选择 `Release`。
3. 生成解决方案。
4. 生成的 `CodeMerger.exe` 在 `x64\Release\` 下。

依赖库（均随源码包含）：

- `SimpleIni`：INI 读写
- `EasySize`：窗口自适应
- 其余为本项目自带的工具头文件

---

## 项目结构

```
CodeMerger/
├─ DllTestor/                    # 主工程
│  ├─ DllTestor.sln
│  ├─ DllTestor.vcxproj
│  ├─ DllTestorDlg.cpp/.h        # 主对话框
│  ├─ TextMergeHelper.cpp/.h     # 编码检测与转换
│  ├─ InPlaceConverter.cpp/.h    # 原地转码（默认关闭）
│  ├─ MyEdit.cpp/.h              # 支持拖拽的 CEdit
│  └─ src/                       # 通用工具（INI、字符串、文件、进度等）
└─ README.md
```

---

## 已知限制

- 单文件默认上限 2 MB，可通过 `MaxFileSizeMB` 调整，但过大会显著增加内存占用。
- 无 BOM 的 UTF-16 通过启发式判断；极短或极端内容的纯 ASCII 文件可能被误判为 ANSI。
  可以在报告中核对，必要时先用 Dry-Run 跑一遍。
- 合并是**一次性顺序读取**，若项目极大（数百 MB 文本）请分目录多次合并。
- 原地转码：
  - 只处理普通文件；隐藏 / 系统文件、只读文件会被跳过并记入报告。
  - 不做跨文件事务回滚；每个文件独立处理，失败只影响该文件。
  - 备份上限 `.bak.99`，超过则跳过并报告。
  - 转换前务必先提交到版本控制系统或整体复制项目。
- 目前只处理文本文件，不处理二进制；若后缀配置有误导致二进制被读入，会在输出中显示为乱码。

---

## 更新日志

### v1.1.0

- 新增原地转码功能（默认关闭，INI 开关）
  - 支持 UTF-8 带 BOM / 不带 BOM 两种目标
  - 自动备份（`.bak` ~ `.bak.99`）
  - Dry-Run 预览模式
  - 生成 `InPlaceConvertReport.md` 并自动打开
- 修复 `IsValidUtf8` 过长编码判断错误（曾导致常用汉字的 UTF-8 无 BOM 文件被误判为 ANSI）
- 增强 UTF-16 无 BOM 识别（启发式 + 兜底手工拼 `wchar_t`）
- 合并时排除 `codeAll.md` 与 `InPlaceConvertReport.md` 自身

### v1.0.0

- 首次开源版本
- 多目录递归合并
- 编码自动检测与统一 UTF-8 输出
- INI 配置持久化
- 拖拽与命令行输入支持

---

## 致谢

- [SimpleIni](https://github.com/brofield/simpleini)
- [EasySize](https://www.codeproject.com/Articles/16581/EasySize)