# Sampler Headless
# Instalação no Raspberry Pi
Este guia detalha o processo de configuração e instalação do Sampler num Raspberry Pi. Siga os passos abaixo cuidadosamente para garantir que tudo funcione corretamente.
## 1. Preparar o Ambiente no Raspberry Pi
Antes de iniciar a instalação, certifique-se de que o Raspberry Pi está atualizado e possui os pacotes necessários instalados.
### Atualizar o Sistema
Execute os seguintes comandos para atualizar os pacotes do sistema:
```
sudo apt update
sudo apt upgrade -y
 ```
### Instalar Dependências Necessárias

Certifique-se de instalar as bibliotecas e ferramentas necessárias para compilar e executar o projeto:
```
sudo apt install -y build-essential git cmake libasound2-dev libcurl4-openssl-dev pkg-config
```

## 2. Clonar o Repositório

Faça o clone do repositório do projeto no Raspberry Pi:
```
git clone https://github.com/kaleberibeiro/Sampler-Headless.git
```

Navegue até a pasta do projeto:
```
cd Sampler-Headless
```
## 3. Configurar o Projeto

Certifique-se de que o caminho para a biblioteca JUCE está configurado corretamente.

### Instalar a Biblioteca JUCE

Se você ainda não possui o JUCE no Raspberry Pi, faça o clone:
```
git clone https://github.com/juce-framework/JUCE.git ~/JUCE
```

## 4. Compilar o Projeto

Use o comando abaixo para compilar o projeto:
```
make
```
Se a compilação ocorrer sem erros, você verá um arquivo executável gerado na pasta do projeto.

## 5. Executar o Projeto

Para executar o StepSequencer, utilize o comando:
```
./StepSequencer
```
## 6. Solução de Problemas

Erro libasound.so.2 => not found: Instale a biblioteca ALSA com o comando:
```
sudo apt install libasound2
```
Erro pkg-config not found: Instale o pkg-config:
```
sudo apt install pkg-config
```
Outros Erros de Compilação: Certifique-se de que todas as dependências estão instaladas e que o caminho para o JUCE está correto no Makefile.

# MIDI Learning
Esta funcionalidade permite uma rápida e fácil configuração dos controladores MIDI.
Para entrar no modo de configuração dos controladores MIDI apenas é necessário correr o programa da seguinte forma:
```
./StepSequencer --midi-learning
```
Após a execução do comando acima, será apresentado na consola a lista de dispositivos MIDI disponíveis e será apenas necessário inserir o índice dos dispositivos a configurar. Por exemplo:
```
Dispositivos MIDI disponíveis:
0: Midi Through Port-0
Insira o(s) índice(s) do(s) dispositivo(s) MIDI que deseja ativar, separados por espaços (ex: 0 2 3):
```
Após a escolha dos dispositivos, irá se decorrer à configuração das funcionalidades. Uma mensagem, por exemplo:

```
Press Play Sequence button...
```
Irá aparecer na consola e o utilizador apenas necessita de pressionar o botão desejado para atribuir a essa função e deverá aparecer a seguinte mensagem de feedback, por exemplo:

```
Assigned CC 43 to action: Play Sequence.
```
Repita o processo para todas as funções e no final a configuração estará completa.

# Funcionalidades
## Sequenciamento
### Samples
8 Samples slots disponíveis.

### Sequências
Cada sample possui um total de 8 sequências que podem ser encadeadas.

### Steps
Cada padrão possui um máximo de 64 passos por sequências, sendo este número modificável.

### Subdivisões Rítmicas
Neste modo é possível dividir um único passo em vários eventos sonoros menores, nomeadamente 2, 3 ou 4. Para tal apenas é necessário entrar nesse modo, selecionar um dos passos da sequência e o número de subdivisões desejadas.

## BPM
Os BPM neste sampler variam de 80 a 207 BPM.

## Manipulação e Efeitos
- Início do sample
- Final do sample
- Envelope ADSR
 - Attack
 - Decay
 - Sustain
 - Release
- Filtro Low-Pass
- Filtro High-Pass
- Tremolo
- Reverb
- Chorus
- Flanger
- Phaser
- Panner

## Finger Drumming
Para ativar o modo Finger Drumming, a sequência necessita de estar parada para então o botão do modo Finger Drumming funcionar.

## Extras
### Armazenamento de Parâmetros
É possível guardar todos os parâmetros e sequências com um único botão.

### Panic Buttons
Este sampler é equipado com dois panic buttons, um que limpa a sequência selecionada e o outro as manipulações do sample selecionado.

## Live Mode
Existe uma opção de Live Mode para este sampler, voltada para performances ao vivo que permite a remoção e inserção de samples ao audio output sem que a sequência pare.

