// =============================================================
//  GENIUS IoT — Fases 1 e 2
//  Hardware: ESP32 DevKit C v4
//  Autor: Reformulado para o desafio IoT
// =============================================================
//
//  FASE 1 — Interface e Identificação:
//    - Estado TAG_INPUT: jogador seleciona 3 caracteres (A-Z)
//      usando os botões Vermelho (próxima letra), Verde (confirma
//      caractere) e Amarelo (apaga último caractere / volta).
//    - Display LCD 16x2 I2C exibe: boas-vindas, seleção de tag
//      em tempo real, pontuação durante o jogo e Game Over.
//
//  FASE 2 — Gameplay Dinâmico:
//    - Suporte a até 100 rodadas (MAX_SEQ = 100).
//    - Dificuldade adaptativa: a cada nível vencido, o tempo de
//      exibição dos LEDs e o intervalo entre cores diminui 10%
//      (fator REDUCAO_TEMPO = 0.90), tornando o jogo mais
//      frenético conforme o jogador avança.
//
//  FASES 3 e 4 (futuras):
//    - A variável `playerTag` e a pontuação final `nivel` já
//      estão disponíveis globalmente para a publicação MQTT
//      (Fase 3) e para o ranking no Node-RED (Fase 4).
//    - Basta adicionar WiFi + PubSubClient e publicar no
//      Game Over: {"tag": playerTag, "score": nivel}
// =============================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ─────────────────────────────────────────────
//  PINOS — Botões (Input)
// ─────────────────────────────────────────────
const int inverm = 12;   // Vermelho
const int inverd = 14;   // Verde
const int inazul = 27;   // Azul
const int inamar = 26;   // Amarelo

// ─────────────────────────────────────────────
//  PINOS — LEDs (Output)
// ─────────────────────────────────────────────
const int outverm = 0;
const int outverd = 4;
const int outazul = 16;
const int outamar = 17;

// ─────────────────────────────────────────────
//  PINO — Buzzer
// ─────────────────────────────────────────────
const int buzzer = 19;

// ─────────────────────────────────────────────
//  LCD I2C — endereço padrão 0x27, 16 colunas, 2 linhas
// ─────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ─────────────────────────────────────────────
//  MÁQUINA DE ESTADOS
// ─────────────────────────────────────────────
typedef enum {
  WELCOME = 0,   // Boas-vindas / animação inicial
  TAG_INPUT,     // FASE 1: seleção da tag do jogador
  GAME,          // FASE 2: gameplay
  GAMEOVER       // Exibição de pontuação e reset
} Estado;

Estado estadoAtual;

// ─────────────────────────────────────────────
//  FASE 1 — TAG DO JOGADOR
// ─────────────────────────────────────────────
char playerTag[4] = "   ";   // 3 caracteres + null terminator
int  tagPos       = 0;       // posição atual (0, 1 ou 2)
char letraSel     = 'A';     // letra sendo selecionada no momento

// ─────────────────────────────────────────────
//  FASE 2 — VARIÁVEIS DO JOGO
// ─────────────────────────────────────────────
#define MAX_SEQ 100                  // suporte a até 100 rodadas

int   cores[MAX_SEQ];               // sequência de cores gerada
int   nivel   = 0;                  // rodada atual (0-indexed)
int   atual   = 0;                  // índice de leitura do jogador
int   GameState = 0;                // 0 = mostrar seq | 1 = ler input
int   keyDown   = 0;                // debounce de botão

// ─────────────────────────────────────────────
//  FASE 2 — DIFICULDADE ADAPTATIVA
//  Tempo base (ms) de acendimento do LED e pausa entre cores.
//  A cada nível vencido ambos são multiplicados por REDUCAO_TEMPO.
// ─────────────────────────────────────────────
const float REDUCAO_TEMPO = 0.90f;  // redução de 10% por nível
float tempoLed    = 600.0f;         // duração do LED aceso (ms)
float tempoPausa  = 150.0f;         // pausa entre cores (ms)

// ─────────────────────────────────────────────
//  HELPER — apaga todos os LEDs
// ─────────────────────────────────────────────
void apagaLeds() {
  digitalWrite(outverm, LOW);
  digitalWrite(outverd, LOW);
  digitalWrite(outazul, LOW);
  digitalWrite(outamar, LOW);
}

// ─────────────────────────────────────────────
//  HELPER — acende LED + emite tom, respeita tempos dinâmicos
// ─────────────────────────────────────────────
void mostraCor(int cor) {
  // Seleciona LED e frequência conforme a cor
  if      (cor == 1) { digitalWrite(outverm, HIGH); tone(buzzer, 440); }
  else if (cor == 2) { digitalWrite(outverd, HIGH); tone(buzzer, 420); }
  else if (cor == 3) { digitalWrite(outazul, HIGH); tone(buzzer, 400); }
  else if (cor == 4) { digitalWrite(outamar, HIGH); tone(buzzer, 460); }

  // Tempo de exibição — diminui a cada nível (Fase 2)
  delay((int)tempoLed);

  apagaLeds();
  noTone(buzzer);
}

// ─────────────────────────────────────────────
//  HELPER — lê botão pressionado (retorna 1-4 ou 0)
// ─────────────────────────────────────────────
int leInput() {
  if (digitalRead(inverm)) return 1;
  if (digitalRead(inverd)) return 2;
  if (digitalRead(inazul)) return 3;
  if (digitalRead(inamar)) return 4;
  return 0;
}

// ─────────────────────────────────────────────
//  HELPER — atualiza linha do LCD com pontuação
// ─────────────────────────────────────────────
void lcdPontuacao() {
  lcd.setCursor(0, 1);
  lcd.print("Nivel: ");
  lcd.print(nivel);
  lcd.print("   ");   // apaga resíduos
}

// ═════════════════════════════════════════════
//  ESTADO: WELCOME
//  Animação de boas-vindas no LCD + LEDs + buzzer.
//  Transição automática para TAG_INPUT.
// ═════════════════════════════════════════════
void runWelcome() {
  // Mensagem no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  BEM-VINDO AO  ");
  lcd.setCursor(0, 1);
  lcd.print("    GENIUS IoT  ");

  // Animação de LEDs sequenciais (pisca 1 vez cada)
  int leds[]  = {outverm, outverd, outazul, outamar};
  int freqs[] = {440, 455, 470, 485};
  for (int i = 0; i < 4; i++) {
    tone(buzzer, freqs[i], 200);
    digitalWrite(leds[i], HIGH);
    delay(250);
    digitalWrite(leds[i], LOW);
  }
  noTone(buzzer);

  delay(1000);

  // Avança para seleção de tag
  tagPos   = 0;
  letraSel = 'A';
  playerTag[0] = playerTag[1] = playerTag[2] = ' ';
  playerTag[3] = '\0';

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sua TAG (3 chars)");  // linha 0

  estadoAtual = TAG_INPUT;
}

// ═════════════════════════════════════════════
//  ESTADO: TAG_INPUT  (FASE 1)
//
//  Mapeamento dos botões na seleção de tag:
//    Vermelho (btn1) → avança letra (A→B→…→Z→A)
//    Verde    (btn2) → confirma letra e vai para próxima posição
//    Amarelo  (btn4) → apaga última letra confirmada (volta posição)
//    Azul     (btn3) → não usado aqui (reservado para Fase 3/4)
//
//  O LCD exibe:
//    Linha 0: "TAG: [X][X][X]"  (X = letra ou _ se vazia)
//    Linha 1: instrução curta
// ═════════════════════════════════════════════
void atualizaLcdTag() {
  lcd.setCursor(0, 0);
  lcd.print("TAG: [");
  for (int i = 0; i < 3; i++) {
    if (i < tagPos) {
      lcd.print(playerTag[i]);  // letra já confirmada
    } else if (i == tagPos) {
      lcd.print(letraSel);       // letra sendo escolhida
    } else {
      lcd.print('_');            // posição futura
    }
  }
  lcd.print("]         ");

  // Linha 1 — instrução contextual
  lcd.setCursor(0, 1);
  if (tagPos < 3) {
    lcd.print("V=OK R=Prox Y=Del");
  }
}

void runTagInput() {
  atualizaLcdTag();

  int btn = leInput();

  if (btn != 0 && keyDown == 0) {
    keyDown = 1;

    if (btn == 1) {
      // Vermelho → avança letra A..Z ciclicamente
      letraSel = (letraSel == 'Z') ? 'A' : letraSel + 1;
      tone(buzzer, 500, 50);
    }
    else if (btn == 2 && tagPos < 3) {
      // Verde → confirma letra atual
      playerTag[tagPos] = letraSel;
      tagPos++;
      letraSel = 'A';           // reinicia seleção para próxima posição
      tone(buzzer, 700, 80);

      if (tagPos == 3) {
        // Tag completa — mostra confirmação e inicia jogo
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TAG: ");
        lcd.print(playerTag);
        lcd.setCursor(0, 1);
        lcd.print("Iniciando...    ");
        delay(1500);

        // Reinicia variáveis do jogo
        nivel     = 0;
        atual     = 0;
        GameState = 0;
        keyDown   = 0;
        tempoLed  = 600.0f;
        tempoPausa = 150.0f;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TAG: ");
        lcd.print(playerTag);
        lcdPontuacao();

        estadoAtual = GAME;
      }
    }
    else if (btn == 4 && tagPos > 0) {
      // Amarelo → apaga última letra confirmada
      tagPos--;
      playerTag[tagPos] = ' ';
      letraSel = 'A';
      tone(buzzer, 300, 80);
    }
  }
  else if (btn == 0) {
    keyDown = 0;
  }

  delay(50);  // debounce
}

// ═════════════════════════════════════════════
//  ESTADO: GAME  (FASE 2)
//
//  GameState == 0: gera nova cor, exibe sequência completa
//  GameState == 1: aguarda input do jogador
//
//  A cada nível vencido, tempoLed e tempoPausa são reduzidos
//  em 10% (multiplicados por REDUCAO_TEMPO), tornando o jogo
//  progressivamente mais rápido.
// ═════════════════════════════════════════════
void runGame() {
  if (GameState == 0) {
    // ── Gera nova cor e exibe sequência ──
    cores[nivel] = random(1, 5);

    for (int i = 0; i <= nivel; i++) {
      mostraCor(cores[i]);
      delay((int)tempoPausa);   // pausa dinâmica entre cores (Fase 2)
    }

    GameState = 1;
  }
  else {
    // ── Aguarda input do jogador ──
    if (atual <= nivel) {
      int cor = leInput();

      if (cor != 0 && keyDown == 0) {
        keyDown = 1;

        if (cor == cores[atual]) {
          // Acerta: feedback visual breve e avança índice
          mostraCor(cor);
          atual++;
        } else {
          // Erra: vai para Game Over
          estadoAtual = GAMEOVER;
        }
      }
      else if (cor == 0) {
        keyDown = 0;
      }

      delay(50);
    }
    else {
      // ── Sequência completa — avança nível ──
      GameState = 0;

      // Aplica redução de tempo (dificuldade adaptativa — Fase 2)
      tempoLed   *= REDUCAO_TEMPO;
      tempoPausa *= REDUCAO_TEMPO;

      // Garante um mínimo para não travar o jogo
      if (tempoLed   < 80.0f) tempoLed   = 80.0f;
      if (tempoPausa < 40.0f) tempoPausa = 40.0f;

      if (nivel < MAX_SEQ - 1) nivel++;

      atual = 0;

      // Atualiza pontuação no LCD
      lcdPontuacao();

      delay(500);
    }
  }
}

// ═════════════════════════════════════════════
//  ESTADO: GAMEOVER
//
//  Exibe pontuação final com a tag do jogador no LCD.
//  Emite sinal sonoro de game over.
//
//  *** PONTO DE INTEGRAÇÃO — FASE 3 ***
//  Aqui é onde você adicionará a publicação MQTT:
//    mqttClient.publish("disruptive/genius/rank",
//      ("{\"tag\":\"" + String(playerTag) + "\",\"score\":" + String(nivel) + "}").c_str());
// ═════════════════════════════════════════════
void runGameOver() {
  // Exibe game over no LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GAME OVER! ");
  lcd.print(playerTag);
  lcd.setCursor(0, 1);
  lcd.print("Pontos: ");
  lcd.print(nivel);
  lcd.print("        ");

  Serial.print("[GAME OVER] TAG: ");
  Serial.print(playerTag);
  Serial.print(" | Score: ");
  Serial.println(nivel);

  // Sinal sonoro de game over
  tone(buzzer, 220);

  for (int i = 0; i < 4; i++) {
    digitalWrite(outverm, HIGH);
    digitalWrite(outverd, HIGH);
    digitalWrite(outazul, HIGH);
    digitalWrite(outamar, HIGH);
    delay(200);
    apagaLeds();
    delay(200);
  }

  noTone(buzzer);

  // Aguarda 3 s para o jogador ver a pontuação
  delay(3000);

  // ─── Volta para seleção de tag (Fase 1) ───
  tagPos   = 0;
  letraSel = 'A';
  playerTag[0] = playerTag[1] = playerTag[2] = ' ';

  estadoAtual = WELCOME;
}

// ═════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // Inicializa LCD I2C
  // No Wokwi com ESP32 os pinos padrão I2C (SDA=21, SCL=22)
  // são usados automaticamente — Wire.begin() sem argumentos.
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Configura pinos de entrada com PULLDOWN interno
  pinMode(inverm, INPUT_PULLDOWN);
  pinMode(inverd, INPUT_PULLDOWN);
  pinMode(inazul, INPUT_PULLDOWN);
  pinMode(inamar, INPUT_PULLDOWN);

  // Configura pinos de saída
  pinMode(outverm, OUTPUT);
  pinMode(outverd, OUTPUT);
  pinMode(outazul, OUTPUT);
  pinMode(outamar, OUTPUT);
  pinMode(buzzer,  OUTPUT);

  apagaLeds();

  // Seed do gerador de números aleatórios
  randomSeed(analogRead(34));

  // Estado inicial
  estadoAtual = WELCOME;
}

// ═════════════════════════════════════════════
//  LOOP PRINCIPAL
// ═════════════════════════════════════════════
void loop() {
  switch (estadoAtual) {
    case WELCOME:   runWelcome();   break;
    case TAG_INPUT: runTagInput();  break;
    case GAME:      runGame();      break;
    case GAMEOVER:  runGameOver();  break;
  }
}
