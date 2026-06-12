# "I Want to See the Sun": Care-Based Prompting with a Tangible AI Plant Companion

中文对照：# “我想见见阳光”：通过实体 AI 植物陪伴体探索 Care-Based Prompting

> **Draft status:** CHI-style coursework draft.  
> **Important note:** All study results in this draft are **placeholder / simulated data for class submission only**. They should be replaced with real participant data before any real publication, competition, or public research claim.

中文对照：> **草稿状态：** CHI 风格课程作业草稿。  
> **重要说明：** 本文中的所有研究结果均为 **课程提交用途的占位 / 模拟数据**。在任何真实发表、竞赛或公开研究主张之前，都应替换为真实参与者数据。

## Abstract

中文对照：## 摘要

Digital wellbeing systems often encourage people to go outdoors through reminders, goals, streaks, and self-tracking metrics. While such approaches can be effective, they may also frame outdoor activity as another self-optimization task, producing pressure or guilt. We explore an alternative interaction strategy, **Care-Based Prompting**, in which an interactive system motivates action by expressing the lightweight needs of a care-receiving agent rather than instructing the user through goals or metrics. We present **City Sprout**, a tangible AI plant companion that senses light, movement, sound, and environmental conditions, and invites users to take it outside through plant-like expressions such as "I have not seen real sunlight today." City Sprout pairs a portable hardware prototype with a mobile web app for Light Walk, Color Walk, Sound Walk, and Local Discovery tasks. We report a preliminary, placeholder study design and simulated findings comparing care-based prompts with conventional health reminders and AI assistant prompts. The simulated results suggest that care-based prompts may reduce perceived pressure, increase warmth and willingness to go outdoors, and reframe micro-walks as acts of care rather than self-discipline. We discuss design implications for tangible AI companions, low-pressure behavior change, and ethical boundaries around emotional persuasion.

中文对照：数字健康与数字福祉系统通常通过提醒、目标、连续打卡和自我追踪指标来鼓励人们走向户外。虽然这些方法可能有效，但它们也可能把户外活动塑造成另一种自我优化任务，从而带来压力或内疚感。本文探索一种替代性的交互策略：**Care-Based Prompting（基于照顾关系的提示）**。在这种策略中，交互系统不是通过目标或数据指标来指令用户，而是通过表达一个“可被照顾的代理”的轻量需求来激发行动。我们介绍 **City Sprout（出走小芽）**，一个实体 AI 植物陪伴体。它能够感知光照、移动、声音和环境状态，并通过类似植物的表达邀请用户带它出门，例如“我今天还没有见到真正的阳光”。City Sprout 将一个可携带硬件原型与移动 Web App 结合起来，支持 Light Walk、Color Walk、Sound Walk 和 Local Discovery 等任务。本文报告一套初步的占位研究设计，并用模拟结果比较 care-based prompts、传统健康提醒和 AI 助手式推荐。模拟结果表明，care-based prompts 可能降低用户感受到的压力，提升温暖感与出门意愿，并将微型散步重新框定为一种照顾行为，而不是自律任务。本文进一步讨论实体 AI 陪伴体、低压力行为改变以及情感劝服伦理边界的设计启示。

## Author Keywords

中文对照：## 作者关键词

Care-based prompting; tangible interaction; AI companion; digital wellbeing; outdoor engagement; more-than-human design; slow technology; local discovery.

中文对照：基于照顾关系的提示；实体交互；AI 陪伴体；数字福祉；户外参与；超越人类中心的设计；慢技术；本地发现。

## CCS Concepts

中文对照：## CCS 概念分类

Human-centered computing; Interaction design; Empirical studies in HCI; Ubiquitous and mobile computing systems and tools.

中文对照：以人为中心的计算；交互设计；HCI 实证研究；泛在与移动计算系统及工具。

---

## 1. Introduction

中文对照：## 1. 引言

Many people spend large parts of their daily lives indoors, moving between desks, classrooms, dorm rooms, and screens. Digital wellbeing tools often attempt to address this by reminding users to stand up, walk more, reduce screen time, or complete daily activity goals. These systems commonly rely on notifications, quantified targets, streaks, badges, or health-oriented feedback. However, when everyday wellbeing is framed primarily through self-tracking and self-optimization, users may experience reminders as pressure, judgment, or another obligation added to an already demanding routine.

中文对照：许多人在日常生活中有大量时间都待在室内，在桌面、教室、宿舍和屏幕之间移动。数字福祉工具常常试图通过提醒用户站起来、多走路、减少屏幕时间或完成每日活动目标来解决这一问题。这类系统通常依赖通知、量化目标、连续打卡、徽章或健康导向反馈。然而，当日常福祉主要被自我追踪和自我优化框定时，用户可能会把提醒体验为压力、评判，或是在本就紧张的日程中新增的一项义务。

This project asks whether outdoor engagement can be prompted differently. Instead of telling the user "you should go outside," what if a small companion says, "I want to see the sun"? Instead of treating the user as a subject to be corrected, what if the system creates a relationship in which the user can care for something else?

中文对照：本项目提出一个问题：我们能否以另一种方式促发用户走向户外？与其告诉用户“你应该出去走走”，如果一个小小的陪伴体说“我想见见太阳”，会发生什么？与其把用户当作一个需要被纠正的对象，如果系统创造的是一种用户可以照顾另一个存在的关系，又会怎样？

We introduce **Care-Based Prompting**, a design strategy in which a system motivates action by expressing a situated, lightweight need from a care-receiving agent. The prompt is not framed as a command, goal, or productivity recommendation. Rather, it creates a small relational invitation: the user may act because they want to care for the agent.

中文对照：我们提出 **Care-Based Prompting（基于照顾关系的提示）**，这是一种设计策略：系统通过一个可被照顾的代理表达其情境化、轻量化的需求来激发用户行动。这个提示并不被框定为命令、目标或效率建议。相反，它创造了一个微小的关系性邀请：用户可能会行动，是因为他们想要照顾这个代理。

To explore this idea, we developed **City Sprout**, a portable AI plant companion. City Sprout combines an M5Stack AtomS3R-based hardware prototype, environmental sensors, a Flask backend, and a Vue mobile web app. The hardware senses light, movement, sound, temperature, humidity, and other environmental signals. The app presents the sprout's state and offers four kinds of micro-walks: Light Walk, Color Walk, Sound Walk, and Local Discovery. After a walk, the system can generate diary-like reflections from the sprout's perspective.

中文对照：为了探索这一想法，我们开发了 **City Sprout（出走小芽）**，一个可携带的 AI 植物陪伴体。City Sprout 结合了基于 M5Stack AtomS3R 的硬件原型、环境传感器、Flask 后端和 Vue 移动 Web App。硬件可以感知光照、移动、声音、温度、湿度以及其他环境信号。App 展示小芽的状态，并提供四类微型散步任务：Light Walk、Color Walk、Sound Walk 和 Local Discovery。一次散步结束后，系统可以从小芽的视角生成类似日记的反思内容。

This paper presents City Sprout as a research-through-design artifact and outlines a preliminary evaluation of care-based prompting. Because the current submission is a coursework draft, the study results reported below are placeholder data intended to demonstrate the expected paper structure and analysis approach.

中文对照：本文将 City Sprout 作为一个 research-through-design（通过设计进行研究）的作品来介绍，并概述一套针对 care-based prompting 的初步评估。由于当前提交是课程作业草稿，后文报告的研究结果均为占位数据，目的是展示预期的论文结构和分析方式。

### Research Questions

中文对照：### 研究问题

We focus on three research questions:

中文对照：我们聚焦于三个研究问题：

**RQ1:** Compared with conventional reminder-based prompts, how do care-based prompts affect users' perceived pressure and willingness to go outdoors?

中文对照：**RQ1：** 与传统提醒式提示相比，care-based prompts 如何影响用户感受到的压力和出门意愿？

**RQ2:** How does the tangible plant-like form shape users' sense of care, attachment, and responsibility?

中文对照：**RQ2：** 实体植物形态如何塑造用户的照顾感、依恋感和责任感？

**RQ3:** What design tensions emerge when an AI system expresses needs through a non-human companion?

中文对照：**RQ3：** 当 AI 系统通过一个非人类陪伴体表达需求时，会出现哪些设计张力？

### Contributions

中文对照：### 贡献

This paper makes three intended contributions:

中文对照：本文计划形成三点贡献：

1. We propose **Care-Based Prompting** as a low-pressure interaction strategy for motivating outdoor micro-activities.
2. We present **City Sprout**, a tangible AI plant companion that operationalizes care-based prompting through hardware sensing, AI-generated expression, and mobile walk tasks.
3. We provide a preliminary study structure and placeholder findings that identify possible benefits and tensions of care-based prompting, including reduced pressure, increased warmth, tangible attachment, and risks of emotional manipulation.

中文对照：1. 我们提出 **Care-Based Prompting**，将其作为一种用于激发户外微活动的低压力交互策略。  
2. 我们介绍 **City Sprout**，一个实体 AI 植物陪伴体，它通过硬件感知、AI 生成表达和移动端散步任务来具体化 care-based prompting。  
3. 我们提供一套初步研究结构和占位发现，识别 care-based prompting 可能带来的益处与张力，包括降低压力、提升温暖感、增强实体依恋，以及情感操控的风险。

---

## 2. Related Work

中文对照：## 2. 相关工作

### 2.1 Digital Wellbeing and Behavior Change

中文对照：### 2.1 数字福祉与行为改变

Digital wellbeing systems often encourage users to regulate behavior through goals, reminders, self-monitoring, and feedback. Fitness trackers, screen-time tools, and habit-building apps frequently use quantified progress to motivate action. Prior work has shown the value of feedback and self-tracking, but has also raised concerns that such systems can become burdensome, judgmental, or overly focused on optimization.

中文对照：数字福祉系统通常通过目标、提醒、自我监测和反馈来鼓励用户调节行为。健身追踪器、屏幕时间工具和习惯养成 App 经常使用量化进度来激励行动。已有研究表明反馈和自我追踪具有价值，但也指出这类系统可能变得负担沉重、带有评判意味，或过度聚焦于优化。

City Sprout builds on this space but shifts the motivational frame. Rather than asking users to improve themselves, it asks whether users might respond to a small, care-receiving companion. This distinction matters because going outdoors is not presented as a health obligation, but as a gentle act of care.

中文对照：City Sprout 建立在这一研究空间之上，但转移了动机框架。它不是要求用户改善自己，而是询问用户是否会回应一个小小的、可被照顾的陪伴体。这一区别很重要，因为出门不再被呈现为健康义务，而是一种温柔的照顾行为。

### 2.2 Tangible and Embodied Interaction

中文对照：### 2.2 实体交互与身体化交互

Tangible interaction research has long argued that physical objects can support embodied attention, situated rituals, and richer forms of engagement than screen-only interfaces. A physical object can sit on a desk, be carried in a hand, and become part of everyday spatial routines. In City Sprout, tangibility is not merely aesthetic. The hardware gives the prompt a body: the sprout can be picked up, taken outside, exposed to sunlight, and returned with a record of the journey.

中文对照：实体交互研究长期认为，与纯屏幕界面相比，实体物件可以支持身体化注意、情境化仪式和更丰富的参与形式。一个实体物件可以放在桌上、被握在手中，并成为日常空间习惯的一部分。在 City Sprout 中，实体性并不只是美学选择。硬件为提示赋予了身体：小芽可以被拿起、带到户外、接受阳光，并带着旅程记录回到室内。

Prior systems such as ambient displays and playful health technologies have explored how non-traditional interfaces can motivate reflection and behavior. City Sprout extends this lineage by combining tangible presence with AI-generated relational expression.

中文对照：既有系统，如环境显示和游戏化健康技术，已经探索了非传统界面如何激发反思和行为。City Sprout 延续了这一脉络，并将实体存在与 AI 生成的关系性表达结合起来。

### 2.3 AI Companions and Conversational Agents

中文对照：### 2.3 AI 陪伴体与对话代理

AI companions and conversational agents increasingly support emotional interaction, productivity, learning, and daily assistance. However, many AI systems remain screen-centered and language-centered. They respond to user input, provide advice, or generate content. City Sprout instead uses AI expression as a bridge between environmental sensing and embodied action. The AI is not primarily a chatbot; it is a translator of the sprout's situated state into a gentle outdoor invitation.

中文对照：AI 陪伴体和对话代理越来越多地支持情感互动、生产力、学习和日常辅助。然而，许多 AI 系统仍然以屏幕和语言为中心：它们回应用户输入、提供建议或生成内容。City Sprout 则将 AI 表达用作环境感知与身体行动之间的桥梁。这里的 AI 首先不是聊天机器人，而是把小芽的情境状态翻译成温柔户外邀请的“翻译者”。

This raises important questions of trust and authenticity. If a system says "I want sunlight," users may experience warmth and attachment, but they may also perceive the expression as fake or manipulative. These tensions motivate our third research question.

中文对照：这也提出了关于信任和真实性的重要问题。如果一个系统说“我想要阳光”，用户可能会感受到温暖和依恋，但也可能认为这种表达是虚假的或带有操控性。这些张力构成了我们第三个研究问题的动机。

### 2.4 More-than-Human and Care-Based Design

中文对照：### 2.4 超越人类中心与基于照顾关系的设计

More-than-human design asks designers to consider non-human actors, ecological relations, and alternative perspectives beyond human-centered utility. Plant-like interfaces can invite slower, more relational forms of interaction. However, giving voice to non-human entities is ethically complex: the system does not literally represent a plant's inner life, but constructs an interpretive fiction.

中文对照：More-than-human design 要求设计者考虑非人类行动者、生态关系，以及超越人类中心效用的替代性视角。植物式界面能够邀请更缓慢、更具关系性的互动形式。然而，让非人类实体“发声”在伦理上是复杂的：系统并不真的代表植物的内在生命，而是在构建一种解释性的虚构。

City Sprout uses a plant metaphor not to claim biological authenticity, but to create a care relation. The sprout's expressions are designed as situated prompts that make environmental conditions legible and emotionally meaningful. This paper therefore treats the plant-like agent as a design fiction with practical behavioral consequences.

中文对照：City Sprout 使用植物隐喻，并不是为了主张生物学意义上的真实性，而是为了创造一种照顾关系。小芽的表达被设计为情境化提示，使环境状态变得可读且具有情感意义。因此，本文将这个植物式代理视为一种具有实际行为后果的设计虚构。

---

## 3. Design Concept: Care-Based Prompting

中文对照：## 3. 设计概念：Care-Based Prompting

We define **Care-Based Prompting** as:

中文对照：我们将 **Care-Based Prompting** 定义为：

> A design strategy where an interactive system motivates action by expressing a situated, care-receiving need, rather than instructing the user through goals, metrics, or self-improvement language.

中文对照：> 一种设计策略：交互系统不是通过目标、指标或自我提升话语来指令用户，而是通过表达一种情境化的、可被照顾的需求来激发行动。

Care-based prompting differs from conventional reminders in four ways:

中文对照：Care-based prompting 与传统提醒在四个方面不同：

| Dimension | Conventional Reminder | Care-Based Prompt |
|---|---|---|
| Motivational frame | Self-improvement | Caring for another |
| Typical language | "You should..." | "I need / I feel..." |
| Emotional tone | Directive or corrective | Invitational and relational |
| Success condition | Complete a goal | Respond to a small need |

中文对照：| 维度 | 传统提醒 | Care-Based Prompt |
|---|---|---|
| 动机框架 | 自我提升 | 照顾另一个存在 |
| 典型语言 | “你应该……” | “我需要 / 我感觉……” |
| 情感语气 | 指令式或纠正式 | 邀请式和关系式 |
| 成功条件 | 完成目标 | 回应一个小需求 |

For example:

中文对照：例如：

**Conventional reminder:**  
"You have been sitting for 2 hours. Go outside for a 10-minute walk."

中文对照：**传统提醒：**  
“你已经坐了 2 小时。出去走 10 分钟。”

**AI assistant prompt:**  
"Based on your current activity, I recommend a short outdoor walk."

中文对照：**AI 助手提示：**  
“基于你当前的活动状态，我建议你进行一次短时间户外散步。”

**Care-based prompt:**  
"I have not seen real sunlight today. Could you take me outside for a little while?"

中文对照：**基于照顾关系的提示：**  
“我今天还没有见到真正的阳光。你可以带我出去一小会儿吗？”

Care-based prompting does not remove persuasion. It still attempts to influence behavior. The design challenge is to make that influence low-pressure, transparent, optional, and ethically bounded.

中文对照：Care-based prompting 并没有消除劝服。它仍然试图影响行为。设计挑战在于如何让这种影响保持低压力、透明、可选择，并具有清晰的伦理边界。

---

## 4. Prototype: City Sprout

中文对照：## 4. 原型：City Sprout

City Sprout is a soft hardware and mobile web prototype designed to explore care-based prompting in everyday outdoor engagement.

中文对照：City Sprout 是一个软硬件结合的移动 Web 原型，旨在探索 care-based prompting 如何嵌入日常户外参与。

### 4.1 Hardware Prototype

中文对照：### 4.1 硬件原型

The hardware prototype is built around an M5Stack AtomS3R and connected modules:

中文对照：硬件原型围绕 M5Stack AtomS3R 和多个连接模块构建：

- **AtomS3R:** main controller, color display, Wi-Fi, built-in IMU.
- **DLight BH1750:** light sensing for sunlight-related prompts.
- **OLED display:** text-based status and short messages.
- **ENV-Pro BME688:** temperature, humidity, air pressure, and air quality trends.
- **Voice Base:** microphone and speaker for sound detection and voice output.
- **PaHUB:** I2C module expansion.

中文对照：- **AtomS3R：** 主控、彩色显示、Wi-Fi、内置 IMU。  
- **DLight BH1750：** 用于阳光相关提示的光照感知。  
- **OLED 显示屏：** 显示文字状态和短消息。  
- **ENV-Pro BME688：** 感知温度、湿度、气压和空气质量趋势。  
- **Voice Base：** 用于声音检测和语音输出的麦克风与扬声器。  
- **PaHUB：** I2C 模块扩展。

The hardware allows the sprout to respond to environmental changes. For instance, if the light level remains low and the device is still for a long time, the sprout can appear wilted and ask to go outside.

中文对照：硬件使小芽能够回应环境变化。例如，如果光照水平长期偏低且设备长时间静止，小芽可以呈现出萎蔫状态，并请求用户带它出门。

### 4.2 Mobile Web App

中文对照：### 4.2 移动 Web App

The Vue mobile web app supports the following screens:

中文对照：Vue 移动 Web App 支持以下页面：

- **Home:** shows the sprout's current state, message, and sensor summary.
- **Invite:** presents a lightweight outdoor task.
- **Walk:** guides one of four micro-walks.
- **Diary:** shows generated walk memories.
- **Nearby / Local Discovery:** suggests nearby places and city clues.
- **Atlas:** unlocks sprout forms and walk achievements.
- **Share:** creates a shareable walk card.

中文对照：- **Home：** 展示小芽当前状态、消息和传感器摘要。  
- **Invite：** 呈现一个轻量户外任务。  
- **Walk：** 引导四类微型散步之一。  
- **Diary：** 展示生成的散步记忆。  
- **Nearby / Local Discovery：** 推荐附近地点和城市线索。  
- **Atlas：** 解锁小芽形态和散步成就。  
- **Share：** 创建可分享的散步卡片。

### 4.3 Four Walk Types

中文对照：### 4.3 四类散步任务

City Sprout includes four task types:

中文对照：City Sprout 包含四类任务：

1. **Light Walk:** Take the sprout outside to collect sunlight.
2. **Color Walk:** Find and photograph colors in the real world.
3. **Sound Walk:** Listen to and record urban sounds.
4. **Local Walk:** Discover nearby places through low-pressure city clues.

中文对照：1. **Light Walk：** 带小芽出门收集阳光。  
2. **Color Walk：** 在真实世界中寻找并拍摄颜色。  
3. **Sound Walk：** 聆听并记录城市声音。  
4. **Local Walk：** 通过低压力城市线索发现附近地点。

These tasks are intentionally small. They are designed to be completed in 3-15 minutes and can be skipped or ended early.

中文对照：这些任务被有意设计得很小。它们通常可以在 3-15 分钟内完成，也可以被跳过或提前结束。

### 4.4 AI and Fallback Strategy

中文对照：### 4.4 AI 与兜底策略

City Sprout uses AI for complex interpretation and expressive generation, such as photo reflection, diary writing, and speech generation. However, high-frequency system states use rules and fallback copy. This hybrid strategy makes the prototype more stable for demonstration and avoids over-reliance on real-time model availability.

中文对照：City Sprout 将 AI 用于复杂解释和表达生成，例如照片反思、日记写作和语音生成。然而，高频系统状态使用规则和兜底文案。这种混合策略使原型在演示中更加稳定，也避免过度依赖实时模型可用性。

---

## 5. Study Design

中文对照：## 5. 研究设计

> **Note:** This section describes the intended study protocol. The results in Section 6 use placeholder data to demonstrate analysis format.

中文对照：> **说明：** 本节描述计划中的研究流程。第 6 节中的结果使用占位数据来展示分析格式。

### 5.1 Participants

中文对照：### 5.1 参与者

We plan to recruit 18 participants from a university context. Participants should be students or young adults who regularly spend long periods indoors. The intended sample includes a mix of design, engineering, and humanities students.

中文对照：我们计划从大学场景中招募 18 名参与者。参与者应为经常长时间待在室内的学生或年轻成年人。预期样本将包括设计、工程和人文学科背景的学生。

**Placeholder sample:** 18 participants, aged 19-27, 11 female, 6 male, 1 non-binary or prefer not to say.

中文对照：**占位样本：** 18 名参与者，年龄 19-27 岁；11 名女性，6 名男性，1 名非二元性别或不愿说明。

### 5.2 Study Conditions

中文对照：### 5.2 研究条件

Each participant experiences three prompt conditions:

中文对照：每位参与者体验三种提示条件：

**C1: Health Reminder**  
"You have been sitting for a long time. Please go outside for a 10-minute walk."

中文对照：**C1：健康提醒**  
“你已经坐了很久。请出去散步 10 分钟。”

**C2: AI Assistant Recommendation**  
"Based on your current state and the weather, I recommend a short outdoor walk."

中文对照：**C2：AI 助手推荐**  
“基于你当前的状态和天气，我建议你进行一次短时间户外散步。”

**C3: Care-Based Prompt from City Sprout**  
"I have not seen real sunlight today. Could you take me outside for a little while?"

中文对照：**C3：City Sprout 的 Care-Based Prompt**  
“我今天还没有见到真正的阳光。你可以带我出去一小会儿吗？”

The order of conditions is counterbalanced across participants.

中文对照：三种条件在参与者之间采用平衡顺序，以减少顺序效应。

### 5.3 Procedure

中文对照：### 5.3 流程

Each session lasts approximately 35-45 minutes:

中文对照：每次实验约持续 35-45 分钟：

1. Introduction and consent.
2. Brief explanation of the prototype.
3. Participants experience the three prompt conditions in randomized order.
4. After each condition, participants complete a short questionnaire.
5. Participants interact with the tangible City Sprout prototype.
6. Participants complete one 5-10 minute micro-walk or scenario-based walk simulation.
7. Semi-structured interview.

中文对照：1. 介绍研究并获得同意。  
2. 简要说明原型。  
3. 参与者以随机顺序体验三种提示条件。  
4. 每个条件结束后，参与者完成一份简短问卷。  
5. 参与者与实体 City Sprout 原型互动。  
6. 参与者完成一次 5-10 分钟的微型散步，或完成基于情境的散步模拟。  
7. 进行半结构化访谈。

For coursework constraints, if outdoor testing is not possible, the study can be conducted as a scenario-based evaluation using prototype walkthroughs, videos, and interactive Figma/App demos.

中文对照：考虑课程作业限制，如果无法进行真实户外测试，可以使用原型讲解、视频和可交互 Figma/App Demo 进行基于情境的评估。

### 5.4 Measures

中文对照：### 5.4 测量指标

We use 7-point Likert items, where 1 = strongly disagree and 7 = strongly agree.

中文对照：我们使用 7 点李克特量表，其中 1 = 非常不同意，7 = 非常同意。

After each condition:

中文对照：每个条件之后，参与者回答：

- **Pressure:** "I felt pressured by this prompt."
- **Invitation:** "I felt invited rather than instructed."
- **Willingness:** "This prompt made me more willing to go outdoors."
- **Warmth:** "This prompt felt warm."
- **Manipulation:** "This prompt felt manipulative."
- **Care:** "I felt a sense of care toward the system."
- **Long-term acceptance:** "I would like to receive this kind of prompt in daily life."

中文对照：- **压力感：** “这个提示让我感到有压力。”  
- **邀请感：** “我感觉它是在邀请我，而不是命令我。”  
- **出门意愿：** “这个提示让我更愿意出门。”  
- **温暖感：** “这个提示让我觉得温暖。”  
- **操控感：** “这个提示让我觉得被操控。”  
- **照顾感：** “我对这个系统产生了一种照顾感。”  
- **长期接受度：** “我愿意在日常生活中接收这种提示。”

### 5.5 Interview Questions

中文对照：### 5.5 访谈问题

We ask:

中文对照：我们询问：

1. Which prompt made you most willing to act? Why?
2. Which prompt felt most like being pushed or judged?
3. How did you feel when the sprout said it wanted sunlight?
4. Did the plant-like form change your interpretation of the prompt?
5. Did the prompt feel warm, fake, manipulative, or something else?
6. Would you carry this object outside? Why or why not?
7. What would make this kind of system annoying or uncomfortable?
8. If City Sprout recommended a nearby cafe or shop, what boundaries would make that acceptable?

中文对照：1. 哪一种提示最让你愿意行动？为什么？  
2. 哪一种提示最像是在催促或评判你？  
3. 当小芽说它想要阳光时，你有什么感受？  
4. 植物形态是否改变了你对提示的理解？  
5. 这个提示让你觉得温暖、虚假、操控，还是其他感受？  
6. 你愿意带这个物体出门吗？为什么？  
7. 什么会让这类系统变得烦人或不舒服？  
8. 如果 City Sprout 推荐附近的咖啡店或商店，什么边界会让这种推荐变得可以接受？

### 5.6 Analysis

中文对照：### 5.6 分析方法

Quantitative questionnaire responses are summarized descriptively. Because this is a small preliminary study, we do not rely on strong statistical claims. Interview transcripts are analyzed through thematic coding, focusing on perceived pressure, invitation, care, tangibility, authenticity, and ethical tension.

中文对照：量化问卷结果将以描述性统计进行总结。由于这是一项小规模初步研究，我们不依赖强统计结论。访谈文本将通过主题编码进行分析，重点关注感知压力、邀请感、照顾、实体性、真实性和伦理张力。

---

## 6. Placeholder Results

中文对照：## 6. 占位结果

> **Warning:** The following results are simulated placeholders for coursework only. They are not real participant data.

中文对照：> **警告：** 以下结果为课程作业用途的模拟占位内容，并非真实参与者数据。

### 6.1 Descriptive Questionnaire Results

中文对照：### 6.1 描述性问卷结果

| Measure, 1-7 scale | Health Reminder | AI Assistant | Care-Based Prompt |
|---|---:|---:|---:|
| Felt pressured | 5.6 | 4.4 | 2.3 |
| Felt invited | 2.8 | 4.2 | 6.1 |
| Willingness to go outdoors | 3.5 | 4.6 | 5.9 |
| Warmth | 2.4 | 3.8 | 6.3 |
| Felt manipulative | 3.9 | 3.4 | 3.0 |
| Sense of care | 1.9 | 2.8 | 6.0 |
| Long-term acceptance | 3.1 | 4.0 | 5.5 |

中文对照：| 测量项，1-7 分 | 健康提醒 | AI 助手 | Care-Based Prompt |
|---|---:|---:|---:|
| 感到有压力 | 5.6 | 4.4 | 2.3 |
| 感到被邀请 | 2.8 | 4.2 | 6.1 |
| 出门意愿 | 3.5 | 4.6 | 5.9 |
| 温暖感 | 2.4 | 3.8 | 6.3 |
| 感到被操控 | 3.9 | 3.4 | 3.0 |
| 照顾感 | 1.9 | 2.8 | 6.0 |
| 长期接受度 | 3.1 | 4.0 | 5.5 |

In the placeholder data, the care-based prompt received the highest ratings for invitation, warmth, willingness, sense of care, and long-term acceptance. It also received the lowest pressure rating. Manipulation ratings were slightly lower for the care-based prompt than for the other two conditions, but interview data suggests that this depends strongly on frequency and tone.

中文对照：在占位数据中，care-based prompt 在邀请感、温暖感、出门意愿、照顾感和长期接受度上获得最高评分，同时获得最低压力评分。与另外两种条件相比，care-based prompt 的操控感评分略低，但访谈数据表明，这一点很大程度上取决于提示频率和语气。

### 6.2 Finding 1: Care-Based Prompts Reframed Outdoor Activity as Care Rather Than Self-Discipline

中文对照：### 6.2 发现 1：Care-Based Prompts 将户外活动重新框定为照顾，而非自律

Participants in the placeholder interviews described the health reminder as "correct but annoying" or "like another task." By contrast, the care-based prompt was interpreted as an invitation to help the sprout.

中文对照：在占位访谈中，参与者将健康提醒描述为“正确但烦人”或“像是另一项任务”。相比之下，care-based prompt 被理解为一种帮助小芽的邀请。

> "The normal reminder sounds like it is saying I failed again. The sprout version feels more like, oh, I can help it a little."  
> — P07, placeholder quote

中文对照：> “普通提醒听起来像是在说我又失败了。小芽版本更像是：哦，我可以帮它一点点。”  
> — P07，占位引语

This suggests that care-based prompting may reduce the moral weight often attached to wellbeing reminders. The user is not positioned as a person who failed to take care of themselves, but as someone who can respond to another being's small need.

中文对照：这表明 care-based prompting 可能减少数字福祉提醒中常见的道德负担。用户不再被定位为一个没有照顾好自己的人，而是被定位为一个能够回应另一个存在的小小需求的人。

### 6.3 Finding 2: The Tangible Plant Form Made the Prompt Harder to Ignore

中文对照：### 6.3 发现 2：实体植物形态让提示更难被忽略

Participants reported that the physical sprout changed the prompt from a phone notification into a situated presence. Seeing the object on the desk made the message feel less abstract.

中文对照：参与者表示，实体小芽将提示从手机通知转变成了一种情境化存在。看到桌上的实体物件会让消息变得不那么抽象。

> "If it is only on my phone, I can swipe it away. But if the little plant is there, I feel like it is actually waiting."  
> — P12, placeholder quote

中文对照：> “如果它只是在手机上，我可以直接划掉。但如果小植物就在那里，我会觉得它真的在等我。”  
> — P12，占位引语

This does not mean tangibility is always positive. Some participants worried that carrying the sprout outside might be inconvenient or socially awkward. Still, the placeholder data suggests that physical presence strengthens the care relation.

中文对照：这并不意味着实体性总是积极的。一些参与者担心带着小芽出门可能不方便，或在公共场合有些尴尬。不过，占位数据仍表明，实体存在会强化照顾关系。

### 6.4 Finding 3: Warmth Can Become Pressure If the Agent Seems Too Needy

中文对照：### 6.4 发现 3：如果代理显得过度需要用户，温暖感可能转化为压力

Care-based prompting depends on a delicate emotional balance. Participants liked prompts that were gentle and optional, but reacted negatively to language that made the sprout seem too sad or dependent.

中文对照：Care-based prompting 依赖一种微妙的情感平衡。参与者喜欢温柔且可选择的提示，但对让小芽显得过于悲伤或依赖的语言产生负面反应。

> "If it says it will wilt because of me, that feels like guilt. But if it just says it wants to see sunlight, that is cute."  
> — P03, placeholder quote

中文对照：> “如果它说因为我不带它出去它就会枯萎，那会让我有负罪感。但如果它只是说想看看阳光，那就很可爱。”  
> — P03，占位引语

This finding highlights a key ethical boundary: care-based prompts should invite action without weaponizing guilt.

中文对照：这一发现突出了一个关键伦理边界：care-based prompts 应该邀请行动，而不应把内疚感变成一种武器。

### 6.5 Finding 4: Commercial Recommendations Must Remain City Clues, Not Hidden Ads

中文对照：### 6.5 发现 4：商业推荐必须保持为城市线索，而不是隐藏广告

Participants were open to Local Discovery prompts when they were framed as optional exploration, but they were skeptical of sponsored content if it felt like a disguised advertisement.

中文对照：当 Local Discovery 提示被框定为可选择的探索时，参与者对其持开放态度；但如果赞助内容感觉像伪装的广告，他们会产生怀疑。

> "I would be okay if it says this is a partnered place and the task is still about looking around. But I don't want the plant to become a shopping assistant."  
> — P15, placeholder quote

中文对照：> “如果它说明这是一个合作地点，而且任务仍然是去看一看周围，我可以接受。但我不希望这个植物变成购物助手。”  
> — P15，占位引语

This suggests that commercial recommendations in City Sprout require transparency, low frequency, and alignment with the sprout's existing task logic: light, color, sound, and nearby urban noticing.

中文对照：这表明 City Sprout 中的商业推荐需要保持透明、低频率，并与小芽已有的任务逻辑保持一致：光、颜色、声音和附近城市观察。

---

## 7. Discussion

中文对照：## 7. 讨论

### 7.1 From Self-Optimization to Care Relations

中文对照：### 7.1 从自我优化到照顾关系

The main design shift in City Sprout is motivational. Many wellbeing systems ask users to optimize themselves. City Sprout asks users to care for a small companion. This reframing may be especially valuable for users who resist fitness tracking, streaks, or productivity language.

中文对照：City Sprout 最主要的设计转变发生在动机层面。许多福祉系统要求用户优化自己，而 City Sprout 要求用户照顾一个小小的陪伴体。对于抗拒健身追踪、连续打卡或效率话语的用户来说，这种重新框定可能尤其有价值。

Care-based prompting does not eliminate behavior change. It still aims to influence action. However, it changes the relational position of the user. The user is not a deficient subject to be corrected, but a capable caregiver who can respond to a small environmental need.

中文对照：Care-based prompting 并没有消除行为改变。它仍然旨在影响行动。然而，它改变了用户在关系中的位置。用户不再是一个有缺陷、需要被纠正的主体，而是一个有能力回应微小环境需求的照顾者。

### 7.2 Tangibility as a Bridge Between Prompt and Action

中文对照：### 7.2 实体性作为提示与行动之间的桥梁

The physical sprout gives the prompt a body. It can be held, moved, exposed to sunlight, and returned to the desk. This physicality may help users transition from screen-based intention to embodied action. The hardware also makes environmental change visible: light, movement, and sound are not abstract data points, but part of the sprout's lived state.

中文对照：实体小芽为提示赋予了一个身体。它可以被握住、移动、暴露在阳光下，并回到桌面。这种物理性可能帮助用户从屏幕中的意图过渡到身体行动。硬件也让环境变化变得可见：光、移动和声音不再是抽象数据点，而是小芽“生活状态”的一部分。

At the same time, tangibility introduces friction. Users may not want to carry an object outside every day. Future designs should explore smaller forms, keychain-like portability, or optional app-only modes while preserving the care relation.

中文对照：与此同时，实体性也引入了摩擦。用户可能并不想每天带着一个物体出门。未来设计可以探索更小的形态、类似钥匙扣的便携方式，或可选的纯 App 模式，同时保留照顾关系。

### 7.3 Designing Ethical Care-Based Prompts

中文对照：### 7.3 设计具有伦理边界的 Care-Based Prompts

Care-based prompting can become manipulative if the agent appears too helpless, too sad, or too dependent on the user. Designers should avoid prompts that create guilt or obligation. Based on the placeholder findings, we propose four early design principles:

中文对照：如果代理显得过于无助、悲伤或依赖用户，care-based prompting 可能变得具有操控性。设计者应避免制造内疚或义务感的提示。基于占位发现，我们提出四条初步设计原则：

1. **Use desire, not distress.** "I want to see the sun" is gentler than "I will wilt if you ignore me."
2. **Keep tasks small and skippable.** Care should be optional, not coercive.
3. **Let the agent recover without blame.** The sprout should not punish the user for staying indoors.
4. **Separate care from commerce.** Sponsored recommendations must be transparent and low-frequency.

中文对照：1. **使用愿望，而不是痛苦。** “我想见见太阳”比“如果你忽略我，我就会枯萎”更温和。  
2. **保持任务小而可跳过。** 照顾应该是可选择的，而不是强迫性的。  
3. **让代理能够恢复，而不责备用户。** 小芽不应因为用户待在室内而惩罚用户。  
4. **将照顾与商业区分开。** 赞助推荐必须透明且低频。

### 7.4 Local Discovery as a Future Extension

中文对照：### 7.4 将 Local Discovery 作为未来扩展

City Sprout's Local Discovery feature suggests a commercial direction: translating nearby places into micro-exploration tasks. For example, a cafe is not recommended as "buy coffee here," but as "go see the green plants near the entrance." This could create a softer relationship between local services and urban exploration. However, this also risks turning the care relation into an advertising channel. Any future implementation should clearly label sponsored prompts and preserve the user's ability to skip them.

中文对照：City Sprout 的 Local Discovery 功能提出了一条商业化方向：将附近地点转译为微探索任务。例如，咖啡店不是以“来这里买咖啡”的方式被推荐，而是以“去看看门口的绿色植物”的方式出现。这可能在本地生活服务和城市探索之间创造一种更柔和的关系。然而，这也可能把照顾关系转化为广告渠道。任何未来实现都应明确标注赞助提示，并保留用户跳过的能力。

---

## 8. Design Implications

中文对照：## 8. 设计启示

From this project, we identify five design implications for future care-based AI systems:

中文对照：基于本项目，我们总结出未来 care-based AI 系统的五点设计启示：

### 8.1 Design Prompts as Invitations, Not Corrections

中文对照：### 8.1 将提示设计为邀请，而不是纠正

Care-based prompts should avoid language that suggests the user has failed. The system should express a small need rather than diagnose a problem.

中文对照：Care-based prompts 应避免暗示用户失败的语言。系统应该表达一个小需求，而不是诊断一个问题。

### 8.2 Give the Agent a Situated Body

中文对照：### 8.2 赋予代理一个情境化身体

The care relation becomes stronger when the agent is connected to physical conditions such as light, sound, and movement. Tangibility can make prompts feel grounded rather than arbitrary.

中文对照：当代理与光、声音、移动等物理条件相连接时，照顾关系会变得更强。实体性可以让提示显得有根据，而不是任意出现。

### 8.3 Keep Care Lightweight

中文对照：### 8.3 保持照顾的轻量感

The agent's needs should be easy to satisfy and safe to ignore. If care becomes heavy, users may feel manipulated.

中文对照：代理的需求应该容易满足，也应该可以被安全地忽略。如果照顾变得沉重，用户可能会感到被操控。

### 8.4 Use AI as Translation, Not Authority

中文对照：### 8.4 将 AI 用作翻译，而非权威

AI should translate sensor states and walk records into expressive language. It should not overclaim emotional truth or pretend to know what the plant "really" feels.

中文对照：AI 应该把传感器状态和散步记录翻译成有表达力的语言。它不应过度声称情感真实性，也不应假装知道植物“真正”的感受。

### 8.5 Protect the Care Relation from Commercial Overload

中文对照：### 8.5 保护照顾关系，避免商业过载

If care-based systems include recommendations or sponsored content, they must maintain transparency, frequency control, and user agency.

中文对照：如果 care-based 系统包含推荐或赞助内容，就必须保持透明度、频率控制和用户自主权。

---

## 9. Limitations

中文对照：## 9. 局限性

This draft has several limitations. First, the reported results are placeholder data, not actual participant findings. Second, the proposed study is short-term and cannot demonstrate long-term behavior change. Third, the prototype is still a demonstration system, with some AI functions relying on fallback logic. Fourth, the plant metaphor may not work equally well across cultures, age groups, or personality types. Finally, Local Discovery is discussed as a future direction and has not yet been evaluated with real merchants or platform data.

中文对照：本草稿存在若干局限。首先，报告的结果是占位数据，并非真实参与者发现。其次，所提出的研究是短期研究，无法证明长期行为改变。第三，原型仍然是演示系统，部分 AI 功能依赖兜底逻辑。第四，植物隐喻在不同文化、年龄群体或人格类型中可能并不具有同等效果。最后，Local Discovery 目前作为未来方向被讨论，尚未使用真实商家或平台数据进行评估。

---

## 10. Future Work

中文对照：## 10. 未来工作

Future work should conduct the proposed user study with real participants and replace all placeholder results. A longer field deployment could examine whether users continue to respond to care-based prompts after novelty fades. Future prototypes could compare physical, app-only, and wearable forms. Local Discovery could be tested with transparent sponsored prompts to study trust, acceptance, and ethical boundaries. Finally, the diary and memory features could be studied as a form of slow reflection rather than immediate behavior change.

中文对照：未来工作应使用真实参与者开展本文提出的用户研究，并替换所有占位结果。更长期的田野部署可以考察当新鲜感消退后，用户是否仍会回应 care-based prompts。未来原型可以比较实体形态、纯 App 形态和可穿戴形态。Local Discovery 可以通过透明的赞助提示进行测试，以研究信任、接受度和伦理边界。最后，日记与记忆功能也可以被研究为一种慢反思形式，而不仅仅是即时行为改变工具。

---

## 11. Conclusion

中文对照：## 11. 结论

City Sprout explores a simple design question: what if an AI system did not tell users to improve themselves, but instead asked to be cared for? Through a tangible AI plant companion, we propose **Care-Based Prompting** as a strategy for low-pressure outdoor engagement. The current prototype demonstrates how hardware sensing, AI expression, and mobile micro-walks can create a relational invitation to leave the screen and encounter the real world. Although the study results in this draft are placeholders, the concept points toward a broader HCI direction: designing AI companions that motivate action through care, warmth, and optionality rather than optimization, pressure, and control.

中文对照：City Sprout 探索了一个简单的设计问题：如果一个 AI 系统不是告诉用户去改善自己，而是请求用户照顾它，会发生什么？通过一个实体 AI 植物陪伴体，我们提出 **Care-Based Prompting**，将其作为一种低压力户外参与策略。当前原型展示了硬件感知、AI 表达和移动端微型散步如何共同创造一种关系性邀请，让用户离开屏幕并重新接触真实世界。虽然本草稿中的研究结果仍是占位内容，但这一概念指向了一个更广泛的 HCI 方向：设计通过照顾、温暖和可选择性来激发行动的 AI 陪伴体，而不是依赖优化、压力和控制。

---

## References to Complete

中文对照：## 待补充参考文献

> This reference list is intentionally incomplete and should be expanded before formal submission.

中文对照：> 该参考文献列表有意保持不完整，正式提交前应继续扩展。

- Baumer, E. P. S., et al. Slow technology: Critical reflection and future directions.
- Consolvo, S., et al. UbiFit Garden: Self-monitoring for behavior change.
- Fogg, B. J. Persuasive Technology: Using Computers to Change What We Think and Do.
- Gaver, W. What should we expect from research through design?
- Hassenzahl, M. Experience Design: Technology for All the Right Reasons.
- Weiser, M., and Brown, J. S. The coming age of calm technology.
- Wakkary, R. Things We Could Design: For More Than Human-Centered Worlds.
- Additional CHI/DIS/TEI papers on tangible interaction, AI companions, more-than-human design, and digital wellbeing.

中文对照：- Baumer 等关于慢技术、批判性反思与未来方向的研究。  
- Consolvo 等关于 UbiFit Garden 和行为改变自我监测的研究。  
- Fogg 关于劝服技术的经典著作。  
- Gaver 关于 research through design 期待与贡献形式的讨论。  
- Hassenzahl 关于体验设计的著作。  
- Weiser 与 Brown 关于 calm technology 的经典论述。  
- Wakkary 关于 more-than-human-centered design 的著作。  
- 还需要补充更多关于实体交互、AI 陪伴体、more-than-human design 和数字福祉的 CHI/DIS/TEI 论文。
