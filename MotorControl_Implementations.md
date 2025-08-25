# MotorControl: Como criar uma nova implementação (ex.: I2C)

Este projeto usa uma interface única `hal::MotorControl` (arquivo `src/hal/MotorControl.hpp`).
A implementação ativa é selecionada no CMake trocando um único arquivo `.cpp` pela
variável `MOTOR_IMPL`.

- Padrão: `src/hal/h_bridge/MotorControl_HBridge.cpp`
- Exemplo alternativo: `src/hal/pwm/MotorControl_Pwm.cpp`

Para usar outra implementação em build time:
```bash
cmake -S . -B build -G Ninja -DMOTOR_IMPL=$PWD/<caminho/para/impl>.cpp
cmake --build build -j
```
Para voltar ao padrão (e limpar valor em cache):
```bash
cmake -S . -B build -G Ninja -U MOTOR_IMPL
```

## Passos para criar uma nova implementação

1. Estrutura de pastas
   - Crie um subdiretório em `src/hal/` para sua implementação, por exemplo:
     - `src/hal/i2c/` para drivers via I2C
     - `src/hal/spi/` para drivers via SPI
   - Nomeie o arquivo como `MotorControl_<Nome>.cpp`, por exemplo:
     - `src/hal/i2c/MotorControl_I2C.cpp`

2. Inclua a interface
   - No novo `.cpp`, inclua `hal/MotorControl.hpp`:
     ```cpp
     #include "hal/MotorControl.hpp"
     ```
   - Use o namespace `hal`.

3. Construtor
   - Implemente o construtor com a mesma assinatura:
     ```cpp
     MotorControl::MotorControl(uint8_t l_pwm, uint8_t l_dirA, uint8_t l_dirB,
                                uint8_t r_pwm, uint8_t r_dirA, uint8_t r_dirB)
     ```
   - Mapeie os parâmetros para seu backend (eles são genéricos). Em um driver I2C,
     você pode reinterpretá-los como:
     - `l_pwm` -> endereço do dispositivo ou ID do barramento
     - `l_dirA`, `l_dirB` -> registradores/flags específicos
     - idem para o lado direito
   - Chame um método interno (ex.: `setup_impl(...)`) para inicialização do backend.

4. Métodos obrigatórios a implementar
   - `void setSpeedLeft(float v)`
   - `void setSpeedRight(float v)`
   - `void stop()`
   - Opcional (mas recomendado): `void arcadeDrive(float forward, float rotate)`
   - Dica: crie auxiliares internos `apply_left(v)`/`apply_right(v)` para normalizar e limitar o valor em `[-1, 1]`.

5. Escalas e saturação
   - Converta `v in [-1,1]` para o formato do seu dispositivo (por exemplo, 8/12/16 bits).
   - Faça clamp para garantir segurança: `mag = min(max(|v|,0),1)`.

6. Backend I2C (exemplo conceitual)
   - Inicialize o barramento (pico-sdk: `i2c_init`, `gpio_set_function` p/ SDA/SCL, pull-ups, etc.).
   - Converta velocidade em duty/registro do driver.
   - Esquema típico para sentido:
     - `v >= 0`: enviar comando de avanço com magnitude `mag`.
     - `v < 0`: enviar comando de ré com magnitude `mag`.
   - Use retries e verificação de erro ao escrever (`i2c_write_blocking`).

7. Selecionar sua implementação
   - Aponte `MOTOR_IMPL` para seu arquivo `.cpp`:
     ```bash
     cmake -S . -B build -G Ninja -DMOTOR_IMPL=$PWD/src/hal/i2c/MotorControl_I2C.cpp
     cmake --build build -j
     ```

8. Boas práticas
   - Logue condições de erro (timeouts I2C, NACKs).
   - Isole conversões (float -> inteiro) em funções utilitárias.
   - Documente no cabeçalho do `.cpp` o mapeamento de pinos/parâmetros e o protocolo.

## Template mínimo (trechos)

```cpp
// src/hal/i2c/MotorControl_I2C.cpp
#include "hal/MotorControl.hpp"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

namespace hal {
static inline float clamp01(float x){ return x<0.f?0.f:(x>1.f?1.f:x);} 

MotorControl::MotorControl(uint8_t l_pwm, uint8_t l_dirA, uint8_t l_dirB,
                           uint8_t r_pwm, uint8_t r_dirA, uint8_t r_dirB)
: l_pwm_(l_pwm), l_dirA_(l_dirA), l_dirB_(l_dirB), r_pwm_(r_pwm), r_dirA_(r_dirA), r_dirB_(r_dirB) {
    // Inicialize seu backend I2C aqui (barramento, pinos SDA/SCL, endereços etc.)
    stop();
}

void MotorControl::setSpeedLeft(float v){ /* enviar via I2C */ }
void MotorControl::setSpeedRight(float v){ /* enviar via I2C */ }
void MotorControl::stop(){ /* enviar comando de parada/coast */ }

void MotorControl::arcadeDrive(float fwd, float rot){
    float l = fwd + rot, r = fwd - rot;
    if (l>1) l=1; if (l<-1) l=-1; if (r>1) r=1; if (r<-1) r=-1;
    setSpeedLeft(l);
    setSpeedRight(r);
}
} // namespace hal
```

Dica: caso seu driver use um protocolo mais complexo, considere criar um wrapper
interno (ex.: `class I2CMotorDriver { ... }`) para encapsular endereços, registradores
e sequências de comando, mantendo `MotorControl` fino e independente do backend.
