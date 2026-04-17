# 🎮 Genius IoT — ESP32

Transformação do clássico jogo **Genius** em um dispositivo IoT conectado, desenvolvido como projeto acadêmico em 4 fases progressivas. Este repositório contém as **Fases 1 e 2** totalmente implementadas, com a base preparada para as Fases 3 e 4.

---

## 📋 Fases do Projeto

| Fase | Descrição | Status |
|------|-----------|--------|
| 1 | Interface com LCD I2C e identificação do jogador por TAG | ✅ Concluída |
| 2 | Gameplay dinâmico com dificuldade adaptativa | ✅ Concluída |
| 3 | Conectividade MQTT — publicação de resultados na nuvem | 🔲 Pendente |
| 4 | Dashboard Node-RED com ranking Top 10 em tempo real | 🔲 Pendente |

---

## 🛠️ Hardware

### Componentes

| Componente | Quantidade | Observação |
|------------|-----------|------------|
| ESP32 DevKit C v4 | 1 | Microcontrolador principal |
| LCD 16x2 com módulo I2C | 1 | Endereço padrão `0x27` |
| LED Vermelho | 1 | Com resistor 1kΩ |
| LED Verde | 1 | Com resistor 1kΩ |
| LED Azul | 1 | Com resistor 1kΩ |
| LED Amarelo | 1 | Com resistor 1kΩ |
| Buzzer passivo | 1 | |
| Push-button | 4 | Cores: vermelho, verde, azul, amarelo |
| Resistor 1kΩ | 4 | Um por LED |
| Protoboard | 1 | |

### Mapa de Pinos

| Pino ESP32 | Componente | Direção |
|-----------|-----------|---------|
| GPIO 12 | Botão Vermelho | INPUT_PULLDOWN |
| GPIO 14 | Botão Verde | INPUT_PULLDOWN |
| GPIO 27 | Botão Azul | INPUT_PULLDOWN |
| GPIO 26 | Botão Amarelo | INPUT_PULLDOWN |
| GPIO 0 | LED Vermelho | OUTPUT |
| GPIO 4 | LED Verde | OUTPUT |
| GPIO 16 | LED Azul | OUTPUT |
| GPIO 17 | LED Amarelo | OUTPUT |
| GPIO 19 | Buzzer | OUTPUT |
| GPIO 21 | LCD SDA | I2C |
| GPIO 22 | LCD SCL | I2C |

---

## 🎯 Fase 1 — Interface e Identificação

Antes de iniciar o jogo, o jogador seleciona uma **TAG de 3 caracteres** (ex: `ABC`, `JOE`) que serve como identificação no ranking.

### Controles na seleção de TAG

| Botão | Ação |
|-------|------|
| 🔴 Vermelho | Avança a letra (A → B → … → Z → A) |
| 🟢 Verde | Confirma a letra e vai para a próxima posição |
| 🟡 Amarelo | Apaga a última letra confirmada |

### O que o LCD exibe

```
TAG: [A__]
V=OK R=Prox Y=Del
```

Após confirmar os 3 caracteres, o LCD exibe `Iniciando...` e o jogo começa.

---

## 🕹️ Fase 2 — Gameplay Dinâmico

### Regras

O jogo exibe uma sequência de cores via LEDs e sons. O jogador deve repetir a sequência pressionando os botões na ordem correta. A cada rodada vencida, uma nova cor é adicionada ao final da sequência.

### Dificuldade Adaptativa

A cada nível vencido, os tempos são reduzidos em **10%**:

```
tempoLed   × 0.90  →  LEDs acendem por menos tempo
tempoPausa × 0.90  →  intervalo entre cores diminui
```

| Parâmetro | Valor inicial | Mínimo |
|-----------|--------------|--------|
| `tempoLed` | 600 ms | 80 ms |
| `tempoPausa` | 150 ms | 40 ms |

O jogo suporta até **100 rodadas** sem limite fixo.

### O LCD durante o jogo

```
TAG: ABC
Nivel: 7
```

### Game Over

Ao errar a sequência, todos os LEDs piscam, o buzzer emite um tom grave e o LCD exibe a pontuação final:

```
GAME OVER! ABC
Pontos: 7
```

Após 3 segundos, o sistema volta para a tela de seleção de TAG.

---

## 🔄 Máquina de Estados

```
WELCOME → TAG_INPUT → GAME → GAMEOVER → WELCOME
```

| Estado | Descrição |
|--------|-----------|
| `WELCOME` | Animação de boas-vindas com LEDs e buzzer |
| `TAG_INPUT` | Seleção dos 3 caracteres da TAG |
| `GAME` | Gameplay — exibe sequência e lê input |
| `GAMEOVER` | Exibe pontuação, aguarda 3s e reinicia |

---

## 📁 Arquivos do Projeto

```
├── sketch.ino       # Código-fonte principal (Arduino/ESP32)
├── diagram.json     # Esquema elétrico do Wokwi
├── libraries.txt    # Dependências de biblioteca (Wokwi)
└── README.md        # Este arquivo
```

---

## 🚀 Como Simular no Wokwi

1. Acesse [wokwi.com](https://wokwi.com) e crie um novo projeto **ESP32**
2. Substitua o conteúdo de `sketch.ino` e `diagram.json` pelos arquivos deste projeto
3. Crie um arquivo `libraries.txt` com o conteúdo:
   ```
   LiquidCrystal I2C
   ```
4. Clique em **Play ▶** — a biblioteca será instalada automaticamente

---

## 📦 Biblioteca Necessária

| Biblioteca | Versão | Instalação |
|-----------|--------|-----------|
| LiquidCrystal I2C | >= 1.1.2 | Via `libraries.txt` no Wokwi ou Gerenciador de Bibliotecas da Arduino IDE |

---

## 🔮 Próximas Fases (Roadmap)

### Fase 3 — MQTT
O ponto de integração já está marcado no código dentro de `runGameOver()`. Basta adicionar:
- Biblioteca `PubSubClient`
- Conexão WiFi com `WiFi.begin(ssid, password)`
- Publicação no tópico `disruptive/genius/rank`:

```json
{"tag": "ABC", "score": 15}
```

### Fase 4 — Node-RED Dashboard
- Fluxo que escuta o tópico MQTT
- Armazenamento do ranking em variável global
- Dashboard com tabela Top 10 ordenada por pontuação

---

## 📝 Licença

Projeto acadêmico — uso livre para fins educacionais.