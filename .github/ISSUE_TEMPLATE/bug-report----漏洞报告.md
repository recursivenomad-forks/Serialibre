---
name: Bug report /  漏洞报告
about: Report a bug or crash
title: ''
labels: bug
assignees: recursivenomad

---

**Bug Report Template**

---

### **1. Describe the bug**

> Provide a clear and concise description of the bug you’re experiencing.  

[Your answer here]

### **2. Screenshots (if applicable)**

> Add screenshots to help illustrate the problem, if applicable.  

[Attach your screenshots here]

### **3. Upload your JSON map file**

> This is essential for reproducing any UI-related issues.  

[Attach your JSON map file here]

### **4. Provide a minimal Arduino program to replicate the issue**

> Sharing a minimal Arduino sketch helps me replicate the environment and setup that causes the bug.  

[Your Arduino sketch here]  

Here’s an example of a minimal program:  

```cpp
void setup() {
    Serial.begin(115200);
    while (!Serial);
}

void loop() {
    Serial.println("/*1,2,3*/");
    delay(50);
}
```

---

### **对于中国用户 / For Chinese users, please delete this part if you speak English**

### **1. 描述问题**

> 请清晰简洁地描述您遇到的问题。  

[请在这里填写您的答案]

### **2. 截图（如果适用）**

> 如果适用，请添加截图以帮助说明您的问题。  

[在此处附加您的截图]

### **3. 上传您的 JSON 地图文件**

> 这是重现任何与 UI 相关问题的必要条件。  

[在此处附加您的 JSON 地图文件]

### **4. 提供一个最小的 Arduino 程序以复现问题**

> 提供一个最小的 Arduino 程序有助于我复现导致问题的环境和设置。  

[请在此填写您的 Arduino 程序]  

程序的基本步骤包括：

```cpp
void setup() {
    Serial.begin(115200);
    while (!Serial);
}

void loop() {
    Serial.println("/*1,2,3*/");
    delay(50);
}
```
