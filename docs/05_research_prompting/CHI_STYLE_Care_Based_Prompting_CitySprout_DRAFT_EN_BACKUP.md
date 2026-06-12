# "I Want to See the Sun": Care-Based Prompting with a Tangible AI Plant Companion

> **Draft status:** CHI-style coursework draft.  
> **Important note:** All study results in this draft are **placeholder / simulated data for class submission only**. They should be replaced with real participant data before any real publication, competition, or public research claim.

## Abstract

Digital wellbeing systems often encourage people to go outdoors through reminders, goals, streaks, and self-tracking metrics. While such approaches can be effective, they may also frame outdoor activity as another self-optimization task, producing pressure or guilt. We explore an alternative interaction strategy, **Care-Based Prompting**, in which an interactive system motivates action by expressing the lightweight needs of a care-receiving agent rather than instructing the user through goals or metrics. We present **City Sprout**, a tangible AI plant companion that senses light, movement, sound, and environmental conditions, and invites users to take it outside through plant-like expressions such as "I have not seen real sunlight today." City Sprout pairs a portable hardware prototype with a mobile web app for Light Walk, Color Walk, Sound Walk, and Local Discovery tasks. We report a preliminary, placeholder study design and simulated findings comparing care-based prompts with conventional health reminders and AI assistant prompts. The simulated results suggest that care-based prompts may reduce perceived pressure, increase warmth and willingness to go outdoors, and reframe micro-walks as acts of care rather than self-discipline. We discuss design implications for tangible AI companions, low-pressure behavior change, and ethical boundaries around emotional persuasion.

## Author Keywords

Care-based prompting; tangible interaction; AI companion; digital wellbeing; outdoor engagement; more-than-human design; slow technology; local discovery.

## CCS Concepts

Human-centered computing; Interaction design; Empirical studies in HCI; Ubiquitous and mobile computing systems and tools.

---

## 1. Introduction

Many people spend large parts of their daily lives indoors, moving between desks, classrooms, dorm rooms, and screens. Digital wellbeing tools often attempt to address this by reminding users to stand up, walk more, reduce screen time, or complete daily activity goals. These systems commonly rely on notifications, quantified targets, streaks, badges, or health-oriented feedback. However, when everyday wellbeing is framed primarily through self-tracking and self-optimization, users may experience reminders as pressure, judgment, or another obligation added to an already demanding routine.

This project asks whether outdoor engagement can be prompted differently. Instead of telling the user "you should go outside," what if a small companion says, "I want to see the sun"? Instead of treating the user as a subject to be corrected, what if the system creates a relationship in which the user can care for something else?

We introduce **Care-Based Prompting**, a design strategy in which a system motivates action by expressing a situated, lightweight need from a care-receiving agent. The prompt is not framed as a command, goal, or productivity recommendation. Rather, it creates a small relational invitation: the user may act because they want to care for the agent.

To explore this idea, we developed **City Sprout**, a portable AI plant companion. City Sprout combines an M5Stack AtomS3R-based hardware prototype, environmental sensors, a Flask backend, and a Vue mobile web app. The hardware senses light, movement, sound, temperature, humidity, and other environmental signals. The app presents the sprout's state and offers four kinds of micro-walks: Light Walk, Color Walk, Sound Walk, and Local Discovery. After a walk, the system can generate diary-like reflections from the sprout's perspective.

This paper presents City Sprout as a research-through-design artifact and outlines a preliminary evaluation of care-based prompting. Because the current submission is a coursework draft, the study results reported below are placeholder data intended to demonstrate the expected paper structure and analysis approach.

### Research Questions

We focus on three research questions:

**RQ1:** Compared with conventional reminder-based prompts, how do care-based prompts affect users' perceived pressure and willingness to go outdoors?

**RQ2:** How does the tangible plant-like form shape users' sense of care, attachment, and responsibility?

**RQ3:** What design tensions emerge when an AI system expresses needs through a non-human companion?

### Contributions

This paper makes three intended contributions:

1. We propose **Care-Based Prompting** as a low-pressure interaction strategy for motivating outdoor micro-activities.
2. We present **City Sprout**, a tangible AI plant companion that operationalizes care-based prompting through hardware sensing, AI-generated expression, and mobile walk tasks.
3. We provide a preliminary study structure and placeholder findings that identify possible benefits and tensions of care-based prompting, including reduced pressure, increased warmth, tangible attachment, and risks of emotional manipulation.

---

## 2. Related Work

### 2.1 Digital Wellbeing and Behavior Change

Digital wellbeing systems often encourage users to regulate behavior through goals, reminders, self-monitoring, and feedback. Fitness trackers, screen-time tools, and habit-building apps frequently use quantified progress to motivate action. Prior work has shown the value of feedback and self-tracking, but has also raised concerns that such systems can become burdensome, judgmental, or overly focused on optimization.

City Sprout builds on this space but shifts the motivational frame. Rather than asking users to improve themselves, it asks whether users might respond to a small, care-receiving companion. This distinction matters because going outdoors is not presented as a health obligation, but as a gentle act of care.

### 2.2 Tangible and Embodied Interaction

Tangible interaction research has long argued that physical objects can support embodied attention, situated rituals, and richer forms of engagement than screen-only interfaces. A physical object can sit on a desk, be carried in a hand, and become part of everyday spatial routines. In City Sprout, tangibility is not merely aesthetic. The hardware gives the prompt a body: the sprout can be picked up, taken outside, exposed to sunlight, and returned with a record of the journey.

Prior systems such as ambient displays and playful health technologies have explored how non-traditional interfaces can motivate reflection and behavior. City Sprout extends this lineage by combining tangible presence with AI-generated relational expression.

### 2.3 AI Companions and Conversational Agents

AI companions and conversational agents increasingly support emotional interaction, productivity, learning, and daily assistance. However, many AI systems remain screen-centered and language-centered. They respond to user input, provide advice, or generate content. City Sprout instead uses AI expression as a bridge between environmental sensing and embodied action. The AI is not primarily a chatbot; it is a translator of the sprout's situated state into a gentle outdoor invitation.

This raises important questions of trust and authenticity. If a system says "I want sunlight," users may experience warmth and attachment, but they may also perceive the expression as fake or manipulative. These tensions motivate our third research question.

### 2.4 More-than-Human and Care-Based Design

More-than-human design asks designers to consider non-human actors, ecological relations, and alternative perspectives beyond human-centered utility. Plant-like interfaces can invite slower, more relational forms of interaction. However, giving voice to non-human entities is ethically complex: the system does not literally represent a plant's inner life, but constructs an interpretive fiction.

City Sprout uses a plant metaphor not to claim biological authenticity, but to create a care relation. The sprout's expressions are designed as situated prompts that make environmental conditions legible and emotionally meaningful. This paper therefore treats the plant-like agent as a design fiction with practical behavioral consequences.

---

## 3. Design Concept: Care-Based Prompting

We define **Care-Based Prompting** as:

> A design strategy where an interactive system motivates action by expressing a situated, care-receiving need, rather than instructing the user through goals, metrics, or self-improvement language.

Care-based prompting differs from conventional reminders in four ways:

| Dimension | Conventional Reminder | Care-Based Prompt |
|---|---|---|
| Motivational frame | Self-improvement | Caring for another |
| Typical language | "You should..." | "I need / I feel..." |
| Emotional tone | Directive or corrective | Invitational and relational |
| Success condition | Complete a goal | Respond to a small need |

For example:

**Conventional reminder:**  
"You have been sitting for 2 hours. Go outside for a 10-minute walk."

**AI assistant prompt:**  
"Based on your current activity, I recommend a short outdoor walk."

**Care-based prompt:**  
"I have not seen real sunlight today. Could you take me outside for a little while?"

Care-based prompting does not remove persuasion. It still attempts to influence behavior. The design challenge is to make that influence low-pressure, transparent, optional, and ethically bounded.

---

## 4. Prototype: City Sprout

City Sprout is a soft hardware and mobile web prototype designed to explore care-based prompting in everyday outdoor engagement.

### 4.1 Hardware Prototype

The hardware prototype is built around an M5Stack AtomS3R and connected modules:

- **AtomS3R:** main controller, color display, Wi-Fi, built-in IMU.
- **DLight BH1750:** light sensing for sunlight-related prompts.
- **OLED display:** text-based status and short messages.
- **ENV-Pro BME688:** temperature, humidity, air pressure, and air quality trends.
- **Voice Base:** microphone and speaker for sound detection and voice output.
- **PaHUB:** I2C module expansion.

The hardware allows the sprout to respond to environmental changes. For instance, if the light level remains low and the device is still for a long time, the sprout can appear wilted and ask to go outside.

### 4.2 Mobile Web App

The Vue mobile web app supports the following screens:

- **Home:** shows the sprout's current state, message, and sensor summary.
- **Invite:** presents a lightweight outdoor task.
- **Walk:** guides one of four micro-walks.
- **Diary:** shows generated walk memories.
- **Nearby / Local Discovery:** suggests nearby places and city clues.
- **Atlas:** unlocks sprout forms and walk achievements.
- **Share:** creates a shareable walk card.

### 4.3 Four Walk Types

City Sprout includes four task types:

1. **Light Walk:** Take the sprout outside to collect sunlight.
2. **Color Walk:** Find and photograph colors in the real world.
3. **Sound Walk:** Listen to and record urban sounds.
4. **Local Walk:** Discover nearby places through low-pressure city clues.

These tasks are intentionally small. They are designed to be completed in 3-15 minutes and can be skipped or ended early.

### 4.4 AI and Fallback Strategy

City Sprout uses AI for complex interpretation and expressive generation, such as photo reflection, diary writing, and speech generation. However, high-frequency system states use rules and fallback copy. This hybrid strategy makes the prototype more stable for demonstration and avoids over-reliance on real-time model availability.

---

## 5. Study Design

> **Note:** This section describes the intended study protocol. The results in Section 6 use placeholder data to demonstrate analysis format.

### 5.1 Participants

We plan to recruit 18 participants from a university context. Participants should be students or young adults who regularly spend long periods indoors. The intended sample includes a mix of design, engineering, and humanities students.

**Placeholder sample:** 18 participants, aged 19-27, 11 female, 6 male, 1 non-binary or prefer not to say.

### 5.2 Study Conditions

Each participant experiences three prompt conditions:

**C1: Health Reminder**  
"You have been sitting for a long time. Please go outside for a 10-minute walk."

**C2: AI Assistant Recommendation**  
"Based on your current state and the weather, I recommend a short outdoor walk."

**C3: Care-Based Prompt from City Sprout**  
"I have not seen real sunlight today. Could you take me outside for a little while?"

The order of conditions is counterbalanced across participants.

### 5.3 Procedure

Each session lasts approximately 35-45 minutes:

1. Introduction and consent.
2. Brief explanation of the prototype.
3. Participants experience the three prompt conditions in randomized order.
4. After each condition, participants complete a short questionnaire.
5. Participants interact with the tangible City Sprout prototype.
6. Participants complete one 5-10 minute micro-walk or scenario-based walk simulation.
7. Semi-structured interview.

For coursework constraints, if outdoor testing is not possible, the study can be conducted as a scenario-based evaluation using prototype walkthroughs, videos, and interactive Figma/App demos.

### 5.4 Measures

We use 7-point Likert items, where 1 = strongly disagree and 7 = strongly agree.

After each condition:

- **Pressure:** "I felt pressured by this prompt."
- **Invitation:** "I felt invited rather than instructed."
- **Willingness:** "This prompt made me more willing to go outdoors."
- **Warmth:** "This prompt felt warm."
- **Manipulation:** "This prompt felt manipulative."
- **Care:** "I felt a sense of care toward the system."
- **Long-term acceptance:** "I would like to receive this kind of prompt in daily life."

### 5.5 Interview Questions

We ask:

1. Which prompt made you most willing to act? Why?
2. Which prompt felt most like being pushed or judged?
3. How did you feel when the sprout said it wanted sunlight?
4. Did the plant-like form change your interpretation of the prompt?
5. Did the prompt feel warm, fake, manipulative, or something else?
6. Would you carry this object outside? Why or why not?
7. What would make this kind of system annoying or uncomfortable?
8. If City Sprout recommended a nearby cafe or shop, what boundaries would make that acceptable?

### 5.6 Analysis

Quantitative questionnaire responses are summarized descriptively. Because this is a small preliminary study, we do not rely on strong statistical claims. Interview transcripts are analyzed through thematic coding, focusing on perceived pressure, invitation, care, tangibility, authenticity, and ethical tension.

---

## 6. Placeholder Results

> **Warning:** The following results are simulated placeholders for coursework only. They are not real participant data.

### 6.1 Descriptive Questionnaire Results

| Measure, 1-7 scale | Health Reminder | AI Assistant | Care-Based Prompt |
|---|---:|---:|---:|
| Felt pressured | 5.6 | 4.4 | 2.3 |
| Felt invited | 2.8 | 4.2 | 6.1 |
| Willingness to go outdoors | 3.5 | 4.6 | 5.9 |
| Warmth | 2.4 | 3.8 | 6.3 |
| Felt manipulative | 3.9 | 3.4 | 3.0 |
| Sense of care | 1.9 | 2.8 | 6.0 |
| Long-term acceptance | 3.1 | 4.0 | 5.5 |

In the placeholder data, the care-based prompt received the highest ratings for invitation, warmth, willingness, sense of care, and long-term acceptance. It also received the lowest pressure rating. Manipulation ratings were slightly lower for the care-based prompt than for the other two conditions, but interview data suggests that this depends strongly on frequency and tone.

### 6.2 Finding 1: Care-Based Prompts Reframed Outdoor Activity as Care Rather Than Self-Discipline

Participants in the placeholder interviews described the health reminder as "correct but annoying" or "like another task." By contrast, the care-based prompt was interpreted as an invitation to help the sprout.

> "The normal reminder sounds like it is saying I failed again. The sprout version feels more like, oh, I can help it a little."  
> — P07, placeholder quote

This suggests that care-based prompting may reduce the moral weight often attached to wellbeing reminders. The user is not positioned as a person who failed to take care of themselves, but as someone who can respond to another being's small need.

### 6.3 Finding 2: The Tangible Plant Form Made the Prompt Harder to Ignore

Participants reported that the physical sprout changed the prompt from a phone notification into a situated presence. Seeing the object on the desk made the message feel less abstract.

> "If it is only on my phone, I can swipe it away. But if the little plant is there, I feel like it is actually waiting."  
> — P12, placeholder quote

This does not mean tangibility is always positive. Some participants worried that carrying the sprout outside might be inconvenient or socially awkward. Still, the placeholder data suggests that physical presence strengthens the care relation.

### 6.4 Finding 3: Warmth Can Become Pressure If the Agent Seems Too Needy

Care-based prompting depends on a delicate emotional balance. Participants liked prompts that were gentle and optional, but reacted negatively to language that made the sprout seem too sad or dependent.

> "If it says it will wilt because of me, that feels like guilt. But if it just says it wants to see sunlight, that is cute."  
> — P03, placeholder quote

This finding highlights a key ethical boundary: care-based prompts should invite action without weaponizing guilt.

### 6.5 Finding 4: Commercial Recommendations Must Remain City Clues, Not Hidden Ads

Participants were open to Local Discovery prompts when they were framed as optional exploration, but they were skeptical of sponsored content if it felt like a disguised advertisement.

> "I would be okay if it says this is a partnered place and the task is still about looking around. But I don't want the plant to become a shopping assistant."  
> — P15, placeholder quote

This suggests that commercial recommendations in City Sprout require transparency, low frequency, and alignment with the sprout's existing task logic: light, color, sound, and nearby urban noticing.

---

## 7. Discussion

### 7.1 From Self-Optimization to Care Relations

The main design shift in City Sprout is motivational. Many wellbeing systems ask users to optimize themselves. City Sprout asks users to care for a small companion. This reframing may be especially valuable for users who resist fitness tracking, streaks, or productivity language.

Care-based prompting does not eliminate behavior change. It still aims to influence action. However, it changes the relational position of the user. The user is not a deficient subject to be corrected, but a capable caregiver who can respond to a small environmental need.

### 7.2 Tangibility as a Bridge Between Prompt and Action

The physical sprout gives the prompt a body. It can be held, moved, exposed to sunlight, and returned to the desk. This physicality may help users transition from screen-based intention to embodied action. The hardware also makes environmental change visible: light, movement, and sound are not abstract data points, but part of the sprout's lived state.

At the same time, tangibility introduces friction. Users may not want to carry an object outside every day. Future designs should explore smaller forms, keychain-like portability, or optional app-only modes while preserving the care relation.

### 7.3 Designing Ethical Care-Based Prompts

Care-based prompting can become manipulative if the agent appears too helpless, too sad, or too dependent on the user. Designers should avoid prompts that create guilt or obligation. Based on the placeholder findings, we propose four early design principles:

1. **Use desire, not distress.** "I want to see the sun" is gentler than "I will wilt if you ignore me."
2. **Keep tasks small and skippable.** Care should be optional, not coercive.
3. **Let the agent recover without blame.** The sprout should not punish the user for staying indoors.
4. **Separate care from commerce.** Sponsored recommendations must be transparent and low-frequency.

### 7.4 Local Discovery as a Future Extension

City Sprout's Local Discovery feature suggests a commercial direction: translating nearby places into micro-exploration tasks. For example, a cafe is not recommended as "buy coffee here," but as "go see the green plants near the entrance." This could create a softer relationship between local services and urban exploration. However, this also risks turning the care relation into an advertising channel. Any future implementation should clearly label sponsored prompts and preserve the user's ability to skip them.

---

## 8. Design Implications

From this project, we identify five design implications for future care-based AI systems:

### 8.1 Design Prompts as Invitations, Not Corrections

Care-based prompts should avoid language that suggests the user has failed. The system should express a small need rather than diagnose a problem.

### 8.2 Give the Agent a Situated Body

The care relation becomes stronger when the agent is connected to physical conditions such as light, sound, and movement. Tangibility can make prompts feel grounded rather than arbitrary.

### 8.3 Keep Care Lightweight

The agent's needs should be easy to satisfy and safe to ignore. If care becomes heavy, users may feel manipulated.

### 8.4 Use AI as Translation, Not Authority

AI should translate sensor states and walk records into expressive language. It should not overclaim emotional truth or pretend to know what the plant "really" feels.

### 8.5 Protect the Care Relation from Commercial Overload

If care-based systems include recommendations or sponsored content, they must maintain transparency, frequency control, and user agency.

---

## 9. Limitations

This draft has several limitations. First, the reported results are placeholder data, not actual participant findings. Second, the proposed study is short-term and cannot demonstrate long-term behavior change. Third, the prototype is still a demonstration system, with some AI functions relying on fallback logic. Fourth, the plant metaphor may not work equally well across cultures, age groups, or personality types. Finally, Local Discovery is discussed as a future direction and has not yet been evaluated with real merchants or platform data.

---

## 10. Future Work

Future work should conduct the proposed user study with real participants and replace all placeholder results. A longer field deployment could examine whether users continue to respond to care-based prompts after novelty fades. Future prototypes could compare physical, app-only, and wearable forms. Local Discovery could be tested with transparent sponsored prompts to study trust, acceptance, and ethical boundaries. Finally, the diary and memory features could be studied as a form of slow reflection rather than immediate behavior change.

---

## 11. Conclusion

City Sprout explores a simple design question: what if an AI system did not tell users to improve themselves, but instead asked to be cared for? Through a tangible AI plant companion, we propose **Care-Based Prompting** as a strategy for low-pressure outdoor engagement. The current prototype demonstrates how hardware sensing, AI expression, and mobile micro-walks can create a relational invitation to leave the screen and encounter the real world. Although the study results in this draft are placeholders, the concept points toward a broader HCI direction: designing AI companions that motivate action through care, warmth, and optionality rather than optimization, pressure, and control.

---

## References to Complete

> This reference list is intentionally incomplete and should be expanded before formal submission.

- Baumer, E. P. S., et al. Slow technology: Critical reflection and future directions.
- Consolvo, S., et al. UbiFit Garden: Self-monitoring for behavior change.
- Fogg, B. J. Persuasive Technology: Using Computers to Change What We Think and Do.
- Gaver, W. What should we expect from research through design?
- Hassenzahl, M. Experience Design: Technology for All the Right Reasons.
- Weiser, M., and Brown, J. S. The coming age of calm technology.
- Wakkary, R. Things We Could Design: For More Than Human-Centered Worlds.
- Additional CHI/DIS/TEI papers on tangible interaction, AI companions, more-than-human design, and digital wellbeing.
