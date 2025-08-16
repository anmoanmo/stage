# 设计模式

## 设计模式的六大原则

```cpp
/* 代码注释速览（可直接贴到头文件顶部）

1) SRP 单一职责原则
   - 定义：一个类/模块只因一种职责变化而变化
   - 识别：方法名出现 and/or；类既做计算又做 I/O；修改一处牵一发动全身
   - 手段：拆类/提取函数/策略或职责下沉到更合适的对象

2) OCP 开闭原则
   - 定义：对扩展开放、对修改关闭（新增功能尽量通过新增类型/组合完成）
   - 识别：每加一个“类型/分支”都要改原有 switch/if
   - 手段：多态/接口 + 工厂/注册表/插件化；数据驱动配置

3) LSP 里氏替换原则
   - 定义：子类型必须能替换父类型，且不破坏程序正确性
   - 识别：派生类“窄化前置条件/放宽后置条件/新增不可见副作用”
   - 手段：契约式设计（前置/后置/不变式）；优先组合而非继承

4) DIP 依赖倒置原则
   - 定义：高层模块不依赖低层细节；二者都依赖抽象
   - 识别：高层直接 new 具体实现、到处 include 具体头文件
   - 手段：面向接口编程、构造/工厂/IOC 注入；Pimpl 降低耦合

5) ISP 接口隔离原则
   - 定义：多个专用小接口优于一个臃肿大接口
   - 识别：接口方法过多；实现类大量“空实现/抛异常”
   - 手段：按用例切分接口；读写分离；能力（Capability）接口

6) LoD 迪米特法则（最少知道）
   - 定义：只与直接朋友通信，避免“链式穿透”内部结构
   - 识别：obj.a().b().c()；为取内部数据而暴露 getter 链
   - 手段：封装转发/提供高层语义方法；依赖倒置传入所需协作者
*/
```

**一、核心概念与最小案例（以 C++ 为例）**

* **SRP 单一职责**
  反例：同一个类既负责“格式化日志”又负责“落盘/网络发送”。
  改造：拆成 `Formatter` 与 `Sink`，`Logger` 只协调。

  ```cpp
  struct LogRecord { /* level, ts, text... */ };

  struct ISink { virtual ~ISink()=default; virtual void write(const std::string& line)=0; };
  struct FileSink : ISink { void write(const std::string& line) override {/*...*/} };

  struct Formatter { std::string format(const LogRecord& r) {/*...*/} };

  class Logger {
  public:
    explicit Logger(std::unique_ptr<ISink> s): sink_(std::move(s)) {}
    void info(std::string msg) { LogRecord r{/*...*/}; sink_->write(fmt_.format(r)); }
  private:
    Formatter fmt_; std::unique_ptr<ISink> sink_;
  };
  ```

  要点：格式变化不影响落地；落地策略变化不影响格式。
* **OCP 开闭原则**
  反例：

  ```cpp
  // 每加一种形状就改这里：违背 OCP
  double area(const Shape& s) {
    if (s.kind==Circle) { /*...*/ }
    else if (s.kind==Rect) { /*...*/ }
    // ...
  }
  ```

  改造：

  ```cpp
  struct Shape { virtual ~Shape()=default; virtual double area() const =0; };
  struct Circle: Shape { double r; double area() const override { return 3.14159*r*r; } };
  // 新增 Triangle 只需新增类型，不改旧代码
  ```

  工具：虚函数/策略/访问者、运行时注册表、配置驱动。
* **LSP 里氏替换**
  经典坑：`Rectangle` 与 `Square` 的继承常破坏子可替换性（设长改宽的语义不一致）。
  建议：用组合表达约束，或让不可替换的类型并列实现共同接口，不强行继承。
* **DIP 依赖倒置**
  反例：

  ```cpp
  class Logger {
    FileSink sink_; // 高层直接依赖具体底层
  };
  ```

  改造（依赖抽象 + 注入）：

  ```cpp
  class Logger {
  public: explicit Logger(std::unique_ptr<ISink> s): sink_(std::move(s)) {}
  private: std::unique_ptr<ISink> sink_;
  };
  // 选择具体实现由工厂/IOC 决定
  ```

  搭配：Pimpl 隐藏细节，降低编译依赖与重编译成本。
* **ISP 接口隔离**
  反例：

  ```cpp
  struct IStorage { virtual read(); virtual write(); virtual watch(); virtual compaction(); /*... 12个方法*/ };
  ```

  改造：

  ```cpp
  struct IReadable { virtual read()=0; };
  struct IWritable { virtual write()=0; };
  struct IWatchable { virtual watch()=0; };
  // 具体类按需实现相应能力接口
  ```
* **LoD 迪米特法则**
  反例：

  ```cpp
  // 业务对象跨越多层拿内部字段
  auto v = order.customer().account().balance();
  ```

  改造：

  ```cpp
  // 提供高层语义方法，隐藏内部结构
  bool Order::canSettleWith(double amount) const;
  ```

  或者将需要的协作者直接注入参数，避免“火车式”链式调用。

**二、工程落地与检查清单**


| 原则 | 关注点       | 代码坏味道            | 重构清单            | 测试要点                   |
| ---- | ------------ | --------------------- | ------------------- | -------------------------- |
| SRP  | 变更来源数量 | 上帝类、方法过长      | 提取类/函数、策略化 | 针对单一职责写小而专的单测 |
| OCP  | 扩展路径     | 不断增长的`switch/if` | 多态/插件/注册表    | 新增类型零改动旧用例       |
| LSP  | 可替换性     | 子类收紧前置条件      | 组合优先、契约检查  | 父接口用例跑在子类上       |
| DIP  | 依赖方向     | 高层 new 具体类       | 抽象 + 注入         | 用桩/Mock 隔离底层 I/O     |
| ISP  | 接口粒度     | “胖接口”、空实现    | 能力接口/读写分离   | 按用例只依赖所需接口       |
| LoD  | 交互半径     | 链式穿透、泄露内部    | 门面/转发/语义方法  | 通过公共 API 完成业务      |

**三、与你的项目示例映射（日志系统 / RPC）**

* 日志系统
  * SRP：`Formatter` 只管“边界到字符串”，`Sink` 只管“字符串到介质”。
  * OCP：新增 `ConsoleSink/AsyncSink/SyslogSink` 以“新增类 + 注册”完成，用例不修改。
  * DIP/ISP：`ISink{write(line)}` 简洁稳定；高级能力另以可选接口扩展（如 `IFlushable`）。
  * LoD：`Logger` 暴露 `info/debug/error` 即可，外界不需要知道队列/线程等内部实现。
  * LSP：所有 `ISink` 必须满足同样的错误语义（例如失败抛出同一异常类别/返回同类状态）。
* RPC
  * OCP：新增编解码、负载均衡策略、序列化协议，采用“策略接口 + 注册”扩展。
  * DIP：`Client` 依赖 `ITransport`/`ICodec` 抽象；可在本地替换为 Mock 做单测。
  * ISP：将“连接管理/请求重试/熔断/度量”拆分为能力接口，避免巨石 `IRpcClient`。
  * LoD：业务侧只见 `call<Service::Fn>(req)`，内部不暴露连接池细节。

**四、取舍与优先级建议**

* 首重 **SRP/DIP/ISP**（降低耦合、明确依赖），随后保障 **OCP/LSP** 的扩展与正确性，最后用 **LoD** 收束交互半径。
* 不要教条：若过度抽象让理解与调试成本跃升，先以 SRP 清晰边界，等变更频率验证后再抽象为稳定接口。

**五、常见第六/第七条的差异说明**

* 本文的“六大原则”采用：**SRP、OCP、LSP、DIP、ISP、LoD**。
* 有些资料将 **合成复用原则（组合/聚合优先于继承）** 计入“六大”，并以 **LoD** 作为第七；两者并不冲突，且实践中“组合优先”与 LSP 一起使用效果更佳。

需要的话，我可以把上面的 Logger/Sink 结构进一步做成一套最小可运行样例（含单测脚手架与注册表插件化示例）。

## 单例模式

### 一、目的与定义（一句话版）

**全局唯一 + 全局可达** 的对象创建模式：控制构造只发生一次，并提供统一访问入口。

---

### 二、什么时候用 / 什么时候不用

* **适用**：轻量全局配置读取、ID 生成器、度量/监控收集器、进程内序列号、（谨慎）日志落地器。
* **不适用**：连接池/线程池/数据库句柄等需要**可替换、可并行多实例**或强生命周期管理的对象（优先**依赖注入**）。

---

### 三、核心风险（牢记）

1. **可测试性差**：隐藏全局状态，难以替换为 Mock。
2. **静态析构顺序**：跨翻译单元（TU）的析构顺序未指定，退出阶段易踩雷。
3. **跨动态库多份实例**：不同 SO/DLL 可能各持一份静态对象。
4. **并发初始化**：需确保**只初始化一次**且线程安全。
5. **跨进程唯一性**：语言层面不保证（需要文件锁/系统互斥等 IPC）。

---

### 四、推荐实现与代码范式（C++11+）

#### 4.1 Meyers Singleton（首选，最小正确）

> 函数内局部静态，C++11 起初始化线程安全；懒加载，极简。

```cpp
// Config.hpp
#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

class Config {
public:
  static Config& Instance() noexcept {
    static Config inst;                // 线程安全的懒初始化（C++11+）
    return inst;
  }

  void Set(std::string k, std::string v) {
    std::lock_guard<std::mutex> lk(mu_);
    kv_[std::move(k)] = std::move(v);
  }
  std::string Get(const std::string& k) const {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = kv_.find(k);
    return it==kv_.end()? "" : it->second;
  }

private:
  Config() = default;
  ~Config() = default;                 // 注意：跨 TU 析构次序不确定
  Config(const Config&) = delete;
  Config& operator=(const Config&) = delete;

  mutable std::mutex mu_;
  std::unordered_map<std::string, std::string> kv_;
};
```

**要点**：避免在**全局/静态对象的析构**期间再次使用 `Instance()`。

#### 4.2 带参数一次性初始化（`std::call_once`）

> 需要在首次使用前注入路径、容量等配置时使用。

```cpp
// SingletonWithInit.hpp
#pragma once
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

template <class T>
class SingletonWithInit {
public:
  template <class... Args>
  static T& Init(Args&&... args) {
    std::call_once(init_flag_, [&] {
      ptr_.reset(new T(std::forward<Args>(args)...));
    });
    return *ptr_;
  }
  static T& Instance() {
    if (!ptr_) throw std::logic_error("Singleton not initialized");
    return *ptr_;
  }
  // 可选：Destroy（慎用，防悬垂）
private:
  static std::unique_ptr<T> ptr_;
  static std::once_flag init_flag_;
};
template <class T> std::unique_ptr<T> SingletonWithInit<T>::ptr_;
template <class T> std::once_flag SingletonWithInit<T>::init_flag_;
```

**使用**：

```cpp
class FileLogger {
public:
  FileLogger(std::string path, size_t qsize);
  void Info(const std::string& msg);
};
using GlobalLogger = SingletonWithInit<FileLogger>;

// 进程启动早期（如 main 开头）：
auto& lg = GlobalLogger::Init("/var/log/app.log", 1<<18);
// 其他位置：
GlobalLogger::Instance().Info("hello");
```

#### 4.3 饿汉式与“泄漏式”（工程折衷）

* **饿汉式**：静态对象在静态期构造，简单但**非懒加载**。
* **泄漏式**：`new` 后不析构，由 OS 回收，**规避析构顺序问题**，代价是工具会报“泄漏”。

---

### 五、反模式与误区

* **双重检查锁（DCLP）**：C++11 前存在重排风险；即使在 C++11+ 使用 `std::atomic` 也不如上述两种方案易审计。
* **服务定位器**伪装下的全局依赖：比单例更难测，除非有强理由，不建议。
* 将**可变业务状态**塞进单例：导致隐藏依赖和耦合扩散。

---

### 六、与日志系统/RPC 的落地建议

* **日志系统**
  * 更优：`App`/IOC 根对象持有 `Logger`，按需注入；第三方回调需要全局时，再提供只读全局入口。
  * 若使用单例：采用 **带参一次性初始化**（路径、异步队列大小），**避免在静态析构期写日志**。
* **RPC**
  * `Codec`、`LoadBalancer`、`RetryPolicy` 用**策略 + 注册表**扩展，不要做成单例。
  * `IdGenerator`/`Metrics` 可单例，但接口尽量**纯函数化**、无副作用。

---

### 七、线程安全与生命周期清单

* 初始化：Meyers / `call_once`，**一次且仅一次**。
* 访问：成员自身加锁或保证无共享可变状态。
* 析构：
  * 不依赖单例在**退出阶段**继续工作；必要时用“泄漏式”。
  * 对外暴露**明确的 Shutdown** 钩子比依赖静态析构更安全。
* 跨模块：确保单例**定义在唯一模块**（常用公共库），避免每个 SO/DLL 都生成一份。
* 跨进程唯一：利用**文件锁/命名互斥量**实现。

---

### 八、与“六大原则”的吻合

* **SRP**：单例也应保持职责单一，不要变“上帝对象”。
* **DIP/ISP**：使用方依赖接口而非直接拿单例；测试时注入替身实现。
* **LoD**：对外提供语义化方法，避免暴露内部细节。

---

### 九、对比与选型表


| 方案                     | 懒加载 | 线程安全     | 可传参 | 析构可控 | 复杂度 | 典型场景                 |
| ------------------------ | ------ | ------------ | ------ | -------- | ------ | ------------------------ |
| Meyers（局部静态）       | 是     | 是（C++11+） | 否     | 否       | 低     | 轻量配置/只读资源        |
| `call_once + unique_ptr` | 是     | 是           | 是     | 可选     | 中     | 日志器、需外参的全局设施 |
| 饿汉式静态对象           | 否     | 是           | 否     | 否       | 低     | 启动即需要、构造轻       |
| 泄漏式（不析构）         | 是     | 是           | 可     | 进程回收 | 低     | 退出期仍可能被动使用     |

---

### 十、实用开发准则（Checklist）

* [ ]  真的需要**全局唯一**吗？优先考虑**依赖注入**。
* [ ]  选择 **Meyers** 或 **call\_once**，拒绝手搓 DCLP。
* [ ]  若需参数，提供一次性 `Init(args...)`，并在 **main 开头**调用。
* [ ]  避免在**静态/全局析构**中使用单例；必要时“泄漏式”。
* [ ]  单测中通过接口**替换单例依赖**；不要让被测代码直接 `::Instance()`。
* [ ]  跨模块放在**公共库**统一导出；避免多份实例。
* [ ]  跨进程唯一 → IPC 锁；跨机器唯一 → 依赖外部服务（如 Redis/Etcd/Snowflake）。

---

### 十一、可直接套用的头注释模板

```cpp
/*
[Singleton 使用约定]
- 初始化：必须在 main() 早期调用 Init(...)（如需参数），否则 Instance() 抛逻辑错误。
- 线程安全：构造期受保护；读写成员请自担并发控制。
- 生命周期：不保证静态析构顺序；严禁在全局/静态对象析构中访问单例。
- 测试：业务代码只依赖 IXXX 接口；单例仅作为默认实现的提供者。
- 模块化：单例定义集中在 common.so；其他模块仅链接和调用。
*/
```

## 工厂模式

### 一、定义与动机（一句话）

把**创建对象**这件事从“使用者”手里拿走，交给**工厂**（一组可扩展的创建逻辑）去做，从而满足 **OCP**（新增产品不改旧代码）与 **DIP**（高层依赖抽象）。

---

### 二、家族谱系与适用场景

* **简单工厂（Static/Simple Factory）**：一个函数/类依据参数返回不同具体产品。
  适合：产品很少、变化不频繁；不追求强扩展性。
* **工厂方法（Factory Method）**：把“创建步骤”下放到子类，每种产品一个具体工厂类。
  适合：产品族会扩展，但每个创建流程相对简单。
* **抽象工厂（Abstract Factory）**：创建**一族相关产品**（保证兼容性/配套）。
  适合：UI 主题、网络栈（TCP/UDP 配套对象）、同一后端的多件组件。
* **注册表工厂（自注册/插件化）**：按字符串/类型 ID 在**运行时**选择并创建产品。
  适合：策略/插件众多、需要配置驱动和热扩展的系统（日志/RPC/编解码/负载均衡）。

---

### 三、最小可用示例（C++11+）

#### 3.1 简单工厂（入门）

```cpp
struct IShape { virtual ~IShape()=default; virtual double area() const=0; };
struct Circle : IShape { double r; explicit Circle(double r):r(r){} double area() const override { return 3.14159*r*r; } };
struct Rect   : IShape { double w,h; Rect(double w,double h):w(w),h(h){} double area() const override { return w*h; } };

enum class Kind { Circle, Rect };

std::unique_ptr<IShape> MakeShape(Kind k) {
  switch(k) {
    case Kind::Circle: return std::make_unique<Circle>(1.0);
    case Kind::Rect:   return std::make_unique<Rect>(2.0,3.0);
  }
  return nullptr;
}
```

要点：简单直接，但每加一种产品要改 `MakeShape`（违背 OCP）。

#### 3.2 工厂方法（每种产品一个工厂）

```cpp
struct IShapeFactory { virtual ~IShapeFactory()=default; virtual std::unique_ptr<IShape> create() const = 0; };
struct CircleFactory : IShapeFactory { std::unique_ptr<IShape> create() const override { return std::make_unique<Circle>(1.0);} };
struct RectFactory   : IShapeFactory { std::unique_ptr<IShape> create() const override { return std::make_unique<Rect>(2.0,3.0);} };

// 使用：IShapeFactory* f = new CircleFactory(); auto s = f->create();
```

要点：扩展靠**新增工厂类**，调用端只面向 `IShapeFactory`。

#### 3.3 抽象工厂（创建配套的一组对象）

```cpp
// 一族“网络”产品：Socket 与 Codec 必须配套
struct ISocket { virtual ~ISocket()=default; };
struct ICodec  { virtual ~ICodec()=default; };

struct INetFactory {
  virtual ~INetFactory()=default;
  virtual std::unique_ptr<ISocket> make_socket() const = 0;
  virtual std::unique_ptr<ICodec>  make_codec()  const = 0;
};

// TCP 家族
struct TcpSocket: ISocket {};
struct TcpCodec : ICodec  {};
struct TcpFactory: INetFactory {
  std::unique_ptr<ISocket> make_socket() const override { return std::make_unique<TcpSocket>(); }
  std::unique_ptr<ICodec>  make_codec()  const override { return std::make_unique<TcpCodec>();  }
};
```

要点：同一工厂内生产**兼容产品族**，防止“混搭”。

#### 3.4 注册表工厂（插件化首选，强烈推荐）

```cpp
#include <unordered_map>
#include <functional>
#include <shared_mutex>
#include <memory>
#include <string>
#include <stdexcept>
#include <utility>

template<class Base, class... Args>
class RegistryFactory {
public:
  using Creator = std::function<std::unique_ptr<Base>(Args...)>;

  static RegistryFactory& instance() {
    static RegistryFactory inst; return inst;
  }

  bool reg(std::string name, Creator c) {
    std::unique_lock<std::shared_mutex> lk(mu_);
    return creators_.emplace(std::move(name), std::move(c)).second;
  }

  std::unique_ptr<Base> create(const std::string& name, Args... args) const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    auto it = creators_.find(name);
    if (it == creators_.end()) throw std::invalid_argument("unknown key: " + name);
    return (it->second)(std::forward<Args>(args)...);
  }

private:
  mutable std::shared_mutex mu_;
  std::unordered_map<std::string, Creator> creators_;
};

// —— 示例：日志 Sink 工厂 —— //
struct ISink { virtual ~ISink()=default; virtual void write(const std::string&)=0; };
struct ConsoleSink : ISink { void write(const std::string& s) override{/*...*/} };
struct FileSink    : ISink { explicit FileSink(std::string path):path_(std::move(path)){} void write(const std::string& s) override{/*...*/} std::string path_; };

using SinkFactory = RegistryFactory<ISink, std::string /*maybe: json cfg*/>;

// 注册（建议在一个 init() 中集中完成）
inline void register_sinks() {
  SinkFactory::instance().reg("console", [](std::string){ return std::make_unique<ConsoleSink>();});
  SinkFactory::instance().reg("file",    [](std::string p){ return std::make_unique<FileSink>(std::move(p));});
}

// 使用
// register_sinks();
// auto sink = SinkFactory::instance().create("file", "/var/log/app.log");
```

要点：按字符串键从注册表拿构造器；**新增产品=新增类+一行注册**，不用改旧代码，天然支持配置驱动与插件。

---

### 四、与相关模式的边界

* **策略模式**：工厂常与策略一起出现——工厂负责“**创建策略**”，策略负责“**封装算法**”。
* **构建者（Builder）**：关注“**复杂对象的组装过程**”，工厂关注“**选择创建哪一类**”。可：工厂 → 返回某个 Builder。
* **依赖注入（DI/IoC）**：进一步把“选择哪种实现”交给容器/配置。**注册表工厂**是轻量 IoC 的常用实现。
* **原型（Prototype）**：通过克隆现有原型创建新对象，也能替代部分工厂用途（特别是深拷贝/性能敏感）。

---

### 五、在你的项目里的落地建议

**日志系统（bitlog）**

* 抽象：`ISink{write}`, `IFormatter{format}`。
* 用**注册表工厂**管理 `Sink` 与 `Formatter`：
  * 新增 `AsyncSink/SyslogSink/RotatingFileSink` 只需注册一次。
  * 从配置 `{"sink":"file","path":"/var/log/app.log"}` 解析键，`SinkFactory.create(key, path)`。
* 单测：对工厂**注入**伪造产品或注册 `MockSink`，对失败键应抛出明确异常。

**RPC**

* 抽象：`ITransport`（TCP/UDP/TLS）、`ICodec`（Pb/Json）、`ILoadBalancer`。
* 用**抽象工厂**保证兼容族（如 `TLSFactory` 同时生成 `TlsTransport + TlsCodec`）。
* 或统一用注册表工厂按键选择：`transport=tls`, `codec=pb`, `lb=round_robin`，利于 A/B 与灰度。

---

### 六、工程细节与踩坑

* **返回值**：用 `std::unique_ptr<Base>`（或 `std::shared_ptr`，按语义）；不要裸指针。
* **参数传递**：Creator 的签名尽量**稳定**，如 `const Config&` / `const json&`；避免频繁修改导致二次扩散。
* **注册时机**：
  * 推荐：在 `main()` 启动阶段显式调用 `register_*()`；
  * 自注册静态对象可能触发**静态初始化顺序问题**（跨 TU 不确定）。
* **线程安全**：注册阶段通常在单线程；若运行期动态注册/卸载，需加锁并考虑读多写少。
* **错误处理**：未知键抛异常或返回 `expected<T,Err>`；**不要**静默返回 `nullptr`。
* **插件加载**：若要 SO/DLL 插件，导出 `register_plugin()`，主程序 `dlopen` 后调用并记录 `dlclose` 时机。

---

### 七、选择指南与对比


| 方案       | 扩展性     | 配置驱动 | 复杂度 | 典型用途                  |
| ---------- | ---------- | -------- | ------ | ------------------------- |
| 简单工厂   | 低         | 弱       | 低     | 少量产品，快速落地        |
| 工厂方法   | 中         | 中       | 中     | 按产品分工厂类            |
| 抽象工厂   | 高（族级） | 中       | 中高   | 生成配套产品族            |
| 注册表工厂 | 高         | 强       | 中     | 策略/插件系统，运行时扩展 |

---

### 八、Checklist（落地自检）

* [ ]  使用方只依赖**抽象接口**，不直接 `new` 具体类（DIP）。
* [ ]  新增产品无需改旧逻辑（OCP）。
* [ ]  选择了合适的工厂变体（简单/方法/抽象/注册表）。
* [ ]  Creator 返回 `unique_ptr`；错误路径有清晰语义（异常/错误码）。
* [ ]  注册时机**可控**（避免静态顺序问题），并考虑线程安全。
* [ ]  有单测覆盖：未知键、注册冲突、并发创建、配置解析失败。

---

### 九、可直接复用的通用“注册表工厂”模板

```cpp
// factory.hpp
#pragma once
#include <unordered_map>
#include <functional>
#include <shared_mutex>
#include <string>
#include <memory>
#include <stdexcept>

template<class Base, class Key=std::string, class... Args>
class Factory {
public:
  using Creator = std::function<std::unique_ptr<Base>(Args...)>;

  static Factory& Instance() { static Factory f; return f; }

  bool Register(Key k, Creator c) {
    std::unique_lock<std::shared_mutex> lk(mu_);
    return creators_.emplace(std::move(k), std::move(c)).second;
  }

  std::unique_ptr<Base> Create(const Key& k, Args... args) const {
    std::shared_lock<std::shared_mutex> lk(mu_);
    auto it = creators_.find(k);
    if (it == creators_.end()) throw std::invalid_argument("Unknown key");
    return (it->second)(std::forward<Args>(args)...);
  }

private:
    mutable std::shared_mutex mu_;
    std::unordered_map<Key, Creator> creators_;
};

// 使用：Factory<ISink, std::string, const Config&>::Instance().Register("file", [](const Config& c){...});
```

## 建造者模式

### 一、定义与动机（一句话）

把**复杂对象的构建过程**从对象本身和使用者中**分离出来**，按**步骤**渐进设置参数并在 `build()` 时一次性校验，得到**不可变**或**一致性已保证**的对象；同样的构建流程可产出**不同表示**（GoF 版本可配 Director）。

---

### 二、什么时候用 / 不用

* **适用**
  * 构造参数多、存在**可选项/互斥项/默认值**与**约束校验**（HTTP 请求、数据库连接、日志器、RPC 客户端配置）。
  * 希望产物**不可变**（线程安全）或构建过程需要**多步**（先设头，再设体，再校验）。
* **不适用**
  * 只有 1–2 个参数、无复杂约束的简单对象（直接构造或工厂即可）。

---

### 三、常见形态

* **Fluent Builder（链式）**：最常见；`builder.setX().setY().build()`。
* **Staged/分阶段 Builder**：用类型系统限制调用顺序（先必填，再可选）。
* **GoF Builder + Director**：Director 固化“搭配/流程”，Builder 负责具体实现。
* **DSL Builder**：把配置写得像小型领域语言（适配配置文件/脚本）。

---

### 四、最小可用示例（C++11+：HTTP 请求）

```cpp
// http_request.hpp
#pragma once
#include <string>
#include <map>
#include <stdexcept>

class HttpRequest {
public:
  // 只读访问
  const std::string& method() const { return method_; }
  const std::string& url()    const { return url_; }
  const std::map<std::string, std::string>& headers() const { return headers_; }
  const std::string& body()   const { return body_; }
  int timeout_ms() const { return timeout_ms_; }

  class Builder {
  public:
    explicit Builder(std::string url) : url_(std::move(url)) {}

    Builder& method(std::string m) { method_ = std::move(m); return *this; }
    Builder& header(std::string k, std::string v) { headers_[std::move(k)] = std::move(v); return *this; }
    Builder& body(std::string b) { body_ = std::move(b); return *this; }
    Builder& timeout_ms(int t) { timeout_ms_ = t; return *this; }

    HttpRequest build() {
      // 统一校验
      if (url_.empty()) throw std::invalid_argument("url empty");
      if (method_.empty()) method_ = "GET";                         // 默认方法
      if ((method_=="GET" || method_=="HEAD") && !body_.empty())
        throw std::invalid_argument("GET/HEAD must not have body");
      if (!headers_.count("Host")) headers_["Host"] = parse_host(url_);
      return HttpRequest(method_, url_, headers_, body_, timeout_ms_);
    }
  private:
    static std::string parse_host(const std::string& url) {
      // 省略：从 http(s)://host[:port]/... 中提取 host
      return "example.com";
    }
    std::string method_, url_, body_;
    std::map<std::string, std::string> headers_;
    int timeout_ms_ = 5000;
  };

private:
  // 产物构造只对 Builder 开放
  HttpRequest(std::string m, std::string u,
              std::map<std::string,std::string> h,
              std::string b, int t)
    : method_(std::move(m)), url_(std::move(u)),
      headers_(std::move(h)), body_(std::move(b)), timeout_ms_(t) {}

  std::string method_, url_, body_;
  std::map<std::string, std::string> headers_;
  int timeout_ms_;
};

// --- 用法 ---
// auto req = HttpRequest::Builder("https://api.example.com/v1/items")
//              .method("POST")
//              .header("Content-Type","application/json")
//              .body(R"({"id":123})")
//              .timeout_ms(3000)
//              .build();
```

**要点**

* Builder 持有**可变**状态；`build()` 产物 `HttpRequest`**不可变**（只读 API）。
* 在 `build()` 里一次性做**默认值填充**与**约束校验**，避免“半成品对象”在系统内流动。

---

### 五、Staged Builder（强制顺序 / 必填项在编译期约束）

```cpp
// 强制：先设 method 再设 url，最后才能 build
struct NeedMethod;
struct NeedUrl;
struct Ready;

template<typename Stage>
class RequestBuilder;

// 阶段1：必须先选方法
template<> class RequestBuilder<NeedMethod> {
public:
  auto method(std::string m) && {
    data_.method = std::move(m);
    return RequestBuilder<NeedUrl>{std::move(data_)};
  }
private:
  struct Data { std::string method, url; } data_;
};

// 阶段2：必须再设 URL
template<> class RequestBuilder<NeedUrl> {
public:
  explicit RequestBuilder(Data d): data_(std::move(d)) {}
  auto url(std::string u) && {
    data_.url = std::move(u);
    return RequestBuilder<Ready>{std::move(data_)};
  }
private:
  struct Data { std::string method, url; } data_;
};

// 阶段3：可 build
template<> class RequestBuilder<Ready> {
public:
  explicit RequestBuilder(Data d): data_(std::move(d)) {}
  HttpRequest build() && {
    // 用前例 HttpRequest 的构造逻辑
    return HttpRequest::Builder(data_.url).method(data_.method).build();
  }
private:
  struct Data { std::string method, url; } data_;
};

// 用法：auto req = RequestBuilder<NeedMethod>{}.method("GET").url("https://x").build();
```

**优点**：把**调用顺序错误**变成**编译错误**。**缺点**：模板样板略多，维护复杂，按需使用。

---

### 六、GoF 版 Director（可复用装配流程）

```cpp
struct RequestDirector {
  // 统一“JSON POST”的构建流程，易于复用/测试
  static HttpRequest JsonPost(std::string url, std::string json, int timeout_ms=3000) {
    return HttpRequest::Builder(std::move(url))
      .method("POST").header("Content-Type","application/json")
      .timeout_ms(timeout_ms).body(std::move(json)).build();
  }
};
// auto req = RequestDirector::JsonPost("https://api/x", R"({"a":1})");
```

---

### 七、与工厂/策略的边界

* **工厂**解决“**创建哪一类**”；**建造者**解决“**如何一步步把这类对象组装好**”。
* 常组合使用：工厂决定产品类型 → 返回对应 **Builder**（或把 Builder 配好）→ `build()` 产物。

---

### 八、在你的项目里的落地建议

1. **日志系统（bitlog）**

   * `Logger::Builder()`：设置 `formatter(...)`、`add_sink(...)`、`level(...)`、`rotation(size, files)`、`async(queue_size, threads)`；`build()` 返回不可变 `Logger`。
   * 优点：把“复杂组合（多 sink/异步/轮转）+ 约束（异步需要队列和线程数）”集中校验。

   ```cpp
   // 伪代码
   auto logger = Logger::Builder()
                   .level(Level::Info)
                   .formatter(std::make_shared<PatternFormatter>("%Y-%m-%d %H:%M:%S [%l] %v"))
                   .add_sink(std::make_shared<ConsoleSink>())
                   .add_sink(RotatingFileSink::Builder().path("/var/log/app.log").max_size(10<<20).files(5).build())
                   .async(1<<20, 2)
                   .build();
   ```
2. **RPC**

   * `RpcClient::Builder()`：`endpoint(...)`、`conn_pool(size)`、`codec("pb")`、`timeout(...)`、`retry(policy)`、`tls(cert, key)`；在 `build()` 校验“启用 TLS 必须给证书”“重试与幂等方法匹配”等。

---

### 九、工程实践 Checklist

* [ ]  **产物不可变**（或只读 API），Builder 仅作临时“装配器”。
* [ ]  **所有默认/互斥/范围约束**在 `build()` 统一校验并给出清晰错误信息。
* [ ]  **必填项**：用构造参数（Builder 的构造函数强制）或 Staged Builder 保证。
* [ ]  **线程安全**：Builder 非线程安全；产物尽量无共享可变状态，或内部自管锁。
* [ ]  **测试**：
  * 正常路径：给出最小/常见配置能成功构建；
  * 失败路径：缺少必填、互斥冲突、非法范围抛出预期异常；
  * 回归：Director/预设流程的稳定性。
* [ ]  **组合其他模式**：配合工厂（选择产品/返回 Builder）、策略（注入算法/策略对象）。

---

### 十、常见误区

* 为简单对象也上 Builder → **样板过多**。
* Builder 暴露了产物的可变引用，造成“**半成品对象**”逃逸。
* `build()` 里不做校验，错误延迟到运行期深处才爆。
* 在多模块中做**静态自注册 Builder**导致**静态初始化顺序**问题（尽量在 `main()` 显式注册/装配）。

---

### 十一、可复用骨架（Fluent Builder 模板）

template<class Product, class Derived>
class FluentBuilder {
public:
Derived& setA(int v) { a_=v; return self(); }
Derived& setB(std::string v) { b_=std::move(v); return self(); }

Product build() {
// 统一校验/默认
if (a_ < 0) throw std::invalid_argument("a must be >=0");
if (b_.empty()) b_ = "default";
return Product(a_, b_);
}
protected:
int a_ = 0; std::string b_;
Derived& self(){ return static_cast<Derived&>(*this); }
};

// 派生具体 Builder：class XBuilder : public FluentBuilder<X, XBuilder> {};

# 过程中的误解


## 不能直接使用作用域运算符调用非静态函数

### 1）错误现象

```cpp
struct A { void foo(); };
A::foo();  // ❌ 编译错误：非静态成员引用必须与特定对象相对
```

### 2）原因与原理（为什么会错）

* **隐式 `this` 原理**：非静态成员函数的真正类型含有一个**隐式的第一个参数**——`this` 指针。调用表达式 `obj.foo()` 或 `ptr->foo()` 会在语义上转成 `A::foo(&obj, ...)`，即自动提供 `this`。而 `A::foo()` 只是**名字限定**，**没有对象表达式**，因此缺少必需的 `this`，语义不完整而报错。
* **类型系统差异**：`&A::foo` 的类型是**指向成员函数的指针**`R (A::*)(Args...)`，它不同于普通函数指针 `R (*)(Args...)`。只有把它与一个对象一起使用（`(obj.*pmf)(...)` 或 `(ptr->*pmf)(...)`），编译器才能补全 `this` 并完成调用。
* **绑定与派发规则**：非静态成员调用需要**对象绑定**以检查 `const/volatile`、引用限定、可能的**虚派发**等规则；`A::foo()` 绕过了“对象→`this`→绑定”的步骤，因而不成立。
* **名字限定≠调用**：作用域运算符 `::` 仅用于**限定名字的归属**（“这是 A 里的 foo”），不负责提供调用所需的对象上下文。

### 3）正确用法（必须有对象）

```cpp
A a;         a.foo();      // ✅ 对象
A* p = &a;   p->foo();     // ✅ 指针
A& r = a;    r.foo();      // ✅ 引用
```

> 只有**静态成员函数**才可以 `A::bar()` 直接调用，因为它们**没有 `this`**。

---

### 4）相关常见错误与“为什么”

#### 4.1 在 `static` 成员函数里用到非静态成员

```cpp
struct A {
  int x;
  static void f(){ x = 1; }  // ❌ 没有 this
};
```

**原理**：静态成员函数**无 `this`**，因此不能直接访问非静态成员。
**修正**

```cpp
void f(){ x = 1; }                    // 让它拥有 this
static void f(A& self){ self.x = 1; } // 或显式传入对象
```

#### 4.2 类外用 `A::x` 访问非静态数据成员

```cpp
struct A{ int x; } a;
int y = A::x;   // ❌
```

**原理**：非静态数据成员属于**对象实例**，没有对象就没有存储单元。
**修正**：`int y = a.x;`

#### 4.3 lambda/回调中未捕获 `this`

```cpp
struct A{
  void foo();
  void reg(){
    auto cb = []{ foo(); };  // ❌ 无 this
  }
};
```

**原理**：该 lambda 不在成员函数调用语境中，未捕获 `this`，因此没有对象可用。
**修正**：`auto cb = [this]{ foo(); };` 或捕获指针 `A* self = this; auto cb = [self]{ self->foo(); };`

#### 4.4 把非静态成员函数当普通函数指针用

```cpp
struct A{ void on(int); } a;
using Cb = void(*)(int);
Cb cb = &A::on;           // ❌ 类型不同，也缺对象
```

**原理**：`&A::on` 是**成员函数指针**，需要对象才能调用，不能直接当作 `void(*)(int)`。
**修正**：`Cb cb = +[](int v){ a.on(v); };`（用 lambda 绑定对象），或把 `on` 设计为 `static` 并在里面转调。

#### 4.5 指向成员的指针未与对象结合

```cpp
struct A{ int x; } a;
int A::* pm = &A::x; // ✅
int v = a.*pm;       // ✅ 必须与对象结合
```

**原理**：成员指针描述“**到成员的偏移/位置**”，只有放到具体对象上才能“落地”。

---

### 5）修复策略（按需选择）

* 函数**确实不依赖对象状态** → 设计成 **`static` 成员函数**或**命名空间自由函数**，即可 `A::bar()` 或 `ns::bar()` 调用。
* 函数**需要对象状态** → 在调用点**拿到对象**（对象/引用/指针），或在静态/回调中**显式传入/捕获对象**。
* 需要将成员函数作为回调 → 使用**lambda 绑定对象**或**自由函数跳板**，不要直接把成员函数指针当作普通函数指针。

---

### 6）快速自检

* 我是否写了 `A::foo()` 而 `foo` 不是 `static`？
* 当前语境是否**没有 `this`**（`static` 函数、自由函数、未捕获的 lambda）？
* 是否把 `&A::foo` 当作普通函数指针用了？
* 访问数据成员是否缺少对象（写成了 `A::x`）？
* 使用成员指针时是否与对象结合了 `.*`/`->*`？

把以上“原理 + 修正”应用到代码中，凡是“非静态成员引用必须与特定对象相对”的报错，都能沿着“**需要对象→提供对象；不需要对象→改成静态/自由函数**”这条主线快速定位并修复。
