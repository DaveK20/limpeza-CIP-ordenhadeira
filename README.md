# 🥛 Sistema CIP para Ordenhadeira Mecânica

> Sistema embarcado de automação do ciclo de limpeza CIP (*Clean-In-Place*) para ordenhadeiras mecânicas, desenvolvido em Arduino Mega. Controla válvulas motorizadas, bombas peristálticas, resistência de aquecimento e sensores de nível — tudo por meio de um painel físico com display LCD e botões de navegação.

![Platform](https://img.shields.io/badge/Platform-Arduino_Mega-00979D?logo=arduino)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal?logo=arduino)
![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=c%2B%2B)
![Version](https://img.shields.io/badge/Version-0.9.3-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 📋 Sobre o Projeto

O processo CIP (*Clean-In-Place*) é o procedimento padrão na indústria leiteira para higienização de ordenhadeiras sem desmontagem do equipamento. Ele envolve a circulação sequencial de água quente, solução alcalina, solução ácida e sanitizante pelo sistema de tubulações e ordenha.

Este projeto **automatiza completamente esse processo**, eliminando a necessidade de intervenção manual durante o ciclo, reduzindo o risco de contaminação e padronizando a limpeza. O operador apenas seleciona o ciclo desejado no painel e o sistema executa todas as etapas de forma autônoma.

O projeto foi desenvolvido e aplicado no **Campus Bom Jesus do Itabapoana do IFF (Instituto Federal Fluminense)**.

---

## ✨ Funcionalidades

### 🔄 Ciclos de Limpeza

- **Ciclo CIP Completo** — executa automaticamente a sequência padrão:
  1. Pré-enxague com água aquecida
  2. Lavagem com solução alcalina + enxague intermediário
  3. Lavagem com solução ácida + enxague intermediário
  4. Aplicação de sanitizante

- **Ciclo Personalizado** — o operador monta sua própria sequência de rotinas pela interface do painel, podendo salvar o ciclo criado na EEPROM para uso futuro. O ciclo salvo persiste mesmo após desligar o sistema.

### 🧪 Dosagem Automática de Produtos

Três bombas peristálticas independentes realizam a dosagem precisa das soluções:

| Bomba | Produto |
|---|---|
| Bomba 1 | Solução Alcalina |
| Bomba 2 | Solução Ácida |
| Bomba 3 | Sanitizante |

O volume de cada produto é calculado automaticamente com base no **fluxo calibrado da bomba (ml/s)** e no **volume do tanque**, garantindo a concentração correta da solução sem medição manual.

### 🌡️ Controle de Temperatura

- Sensor **DS18B20** monitora a temperatura da água em tempo real
- Resistência elétrica de aquecimento é ativada e desativada automaticamente para manter a temperatura alvo de cada etapa
- Controle com histerese de ±2 °C para evitar acionamentos excessivos da resistência
- Temperaturas padrão por etapa (ajustáveis pelo painel):
  - Pré-enxague: 39 °C
  - Solução alcalina: 39 °C (configurável até 75 °C)
  - Solução ácida: 39 °C (configurável até 55 °C)

### 🚿 Controle de Nível por Boias

Dois sensores de boia controlam o enchimento dos tanques:

- **Boia do tanque de aquecimento** — aciona o enchimento e libera o esvaziamento apenas após atingir o nível correto
- **Boia do tanque de mistura** — monitora o nível do tanque onde as soluções são diluídas

O sistema nunca esvaziará um tanque que não esteja cheio, evitando falhas no processo.

### 🔧 Controle de Válvulas Esféricas Motorizadas

Quatro válvulas motorizadas direcionam o fluxo de água e soluções durante o ciclo:

| Válvula | Função |
|---|---|
| `vs_ciclo` | Direciona o fluxo entre tanque de mistura e circuito de ordenha |
| `vs_vasao` | Direciona o fluxo para saída externa ou tanque de mistura |
| `vs_ts` | Controla entrada de água no tanque de aquecimento |
| `vs_tm` | Controla entrada de água no tanque de mistura |

Todas as válvulas têm tempo de posicionamento respeitado automaticamente (10 segundos), garantindo que estejam completamente abertas ou fechadas antes da próxima ação.

### 🎛️ Painel de Controle Físico

Interface completa com **5 botões** e **display LCD 16x2 I2C**:

| Botão | Função |
|---|---|
| ← Seta Esquerda | Navegar para a opção anterior |
| → Seta Direita | Navegar para a próxima opção |
| ✔ OK | Confirmar seleção |
| ✖ Remover | Remover etapa / decrementar valor |
| ⛔ Interromper | Interrupção imediata do ciclo (hardware interrupt) |

O display exibe em tempo real a etapa em execução, temperatura atual e alertas de status. Textos são centralizados automaticamente no LCD.

### ⚙️ Configurações Ajustáveis pelo Painel

O operador pode configurar e salvar os parâmetros diretamente no painel, sem necessidade de reprogramação:

- **Volume das soluções** (alcalina, ácida e sanitizante) em mL, com incremento de 0,1 mL
- **Temperatura alvo** de cada etapa do ciclo em °C
- **Tempo de circulação** da solução na ordenhadeira (1 a 60 minutos)

Todos os parâmetros são salvos na **EEPROM** do Arduino, persistindo entre reinicializações. A conversão float↔EEPROM é feita com resolução de 0,05 mL por passo.

### 🛡️ Segurança e Robustez

- **Watchdog Timer (WDT)** com reset automático em 8 segundos — o sistema se reinicia automaticamente em caso de travamento
- **Interrupção de emergência por hardware** (pino 2) — o operador pode parar qualquer ciclo instantaneamente a qualquer momento, com esvaziamento seguro do tanque
- **`safeDelay()`** — função de delay com reset do watchdog integrado, evitando reinicializações indesejadas durante esperas longas
- **Detecção de falha no sensor de temperatura** — LED de alerta acende e os ciclos CIP são bloqueados caso o DS18B20 não seja detectado
- **Confirmação dupla** antes de iniciar ciclos — o operador precisa pressionar OK duas vezes dentro de um tempo limite para confirmar a execução

---

## 🛠️ Hardware Utilizado

| Componente | Quantidade | Função |
|---|---|---|
| **Arduino Mega 2560** | 1 | Microcontrolador principal |
| **Sensor DS18B20** | 1 | Temperatura da água (OneWire) |
| **Válvula esférica motorizada** | 4 | Direcionamento do fluxo |
| **Bomba peristáltica** | 3 | Dosagem de alcalino, ácido e sanitizante |
| **Sensor de boia** | 2 | Controle de nível dos tanques |
| **Resistência elétrica** | 1 | Aquecimento da água |
| **Módulo Relé** | 9 canais | Acionamento de todos os atuadores |
| **Display LCD 16x2 I2C** | 1 | Interface com o operador |
| **Push buttons** | 5 | Navegação e controle |
| **LED de status** | 1 | Indicador de falha do sensor |

---

## 🗺️ Mapeamento de Pinos (Arduino Mega)

```
Pino  │ Componente
──────┼──────────────────────────────────
  2   │ Botão Interromper (INT0 - Hardware Interrupt)
  3   │ Botão Remover
  4   │ Botão Seta Direita
  5   │ Botão OK
  6   │ Botão Seta Esquerda
 14   │ Sensor DS18B20 (OneWire)
 17   │ LED Status Sensor
 26   │ Relé Bomba Ácida
 28   │ Relé Bomba Sanitizante
 30   │ Relé Bomba Alcalina
 32   │ Válvula vs_ciclo
 34   │ Válvula vs_vasao
 36   │ Válvula vs_ts (tanque aquecimento)
 38   │ Válvula vs_tm (tanque mistura)
 40   │ Controle Sucção Ordenhadeira
 42   │ Boia tanque de aquecimento
 44   │ Boia tanque de mistura
 46   │ Relé Resistência de Aquecimento
SDA/SCL │ Display LCD I2C (endereço 0x27)
```

---

## 🔁 Fluxo do Ciclo CIP Completo

```
[INÍCIO]
   │
   ▼
[Pré-Enxague]
  Posiciona vs_vasao → Enchimento do tanque → Aquecimento → Sucção → Despejo
   │
   ▼
[Solução Alcalina]
  Dosagem bomba alcalina → Enchimento → Aquecimento → Circulação (tempo config.) → Despejo
   │
   ▼
[Enxague Intermediário]
  Enchimento tanque de mistura → Sucção → Despejo
   │
   ▼
[Solução Ácida]
  Dosagem bomba ácida → Enchimento → Aquecimento → Circulação → Despejo
   │
   ▼
[Enxague Intermediário]
   │
   ▼
[Sanitizante]
  Dosagem bomba sanitizante → Enchimento → Despejo
   │
   ▼
[FIM]
```

> ⚠️ A qualquer momento o operador pode pressionar o **botão de emergência** para interromper o ciclo. O sistema desativa a resistência e esvazia o tanque com segurança antes de retornar ao menu.

---

## 📦 Dependências

```ini
lib_deps =
    milesburton/DallasTemperature
    paulstoffregen/OneWire
    marcoschwartz/LiquidCrystal_I2C
    avr/wdt.h   ; nativa do AVR-GCC
    EEPROM      ; nativa do Arduino
```

---

## 🚀 Como Usar

**1. Clone o repositório:**
```bash
git clone https://github.com/DaveK20/SEU_REPO.git
cd SEU_REPO
```

**2. Abra no PlatformIO ou Arduino IDE e faça o upload para o Arduino Mega.**

**3. Navegação no painel:**
- Use ← → para navegar entre as opções do menu
- Pressione **OK** para selecionar → pressione **OK novamente** dentro de 5 segundos para confirmar
- Pressione **⛔ Interromper** a qualquer momento para parar o ciclo com segurança

**4. Primeiro uso — configure os parâmetros:**
- Menu `Alterar > Temperatura` — defina as temperaturas de cada etapa
- Menu `Alterar > Solucao` — defina o volume de cada produto (mL/L de solução)
- Menu `Alterar > Circulacao` — defina o tempo de circulação (minutos)

---

## 📁 Estrutura do Projeto

```
cip-ordenhadeira/
├── src/
│   └── main.cpp          # Código principal (~1386 linhas)
├── platformio.ini         # Configuração do projeto
└── README.md
```

---

## 📚 Referência Técnica

Os parâmetros de temperatura das soluções foram baseados no artigo:

> **SANTOS, Marcos Veiga.** *Limpeza e Desinfecção de Equipamentos de Ordenha e Tanques de Resfriamento de Leite.* USP/FMVZ.

---
