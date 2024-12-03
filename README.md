# Sampler Headless
## Instalação no Raspberry Pi
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

## MIDI Learning
Esta funcionalidade permite uma rápida e fácil configuração dos controladores MIDI.
Para entrar no modo de configuração dos controladores MIDI apenas é necessário correr o programa da seguinte forma:
```
./StepSequencer --midi-learning
```

