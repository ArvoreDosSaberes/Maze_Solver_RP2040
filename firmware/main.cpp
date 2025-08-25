/**
 * @file main.cpp
 * @brief Firmware principal do carrinho resolvedor de labirintos (RP2040).
 *
 * - Ativa USB CDC e aguarda 3s para comandos de gerenciamento de memória (RESET/STATUS).
 * - Faz logging na serial de cada decisão do navegador com nota 0..10.
 * - Integra HAL de motores (PWM) e sensores IR (ADC) e o núcleo de navegação.
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "core/Navigator.hpp"
#include "core/PersistentMemory.hpp"
#include "hal/IRSensorArray.hpp"
#include "hal/MotorControl.hpp"

using namespace maze;

// -----------------------------
// Compile-time configurable parameters (defaults below can be overridden by -DCFG_* or CMake)
/**
 * @name Parâmetros de configuração (CFG_*)
 * @brief Macros de configuração do firmware ajustáveis em tempo de compilação.
 *
 * Podem ser sobrepostos via opções `-D` no CMake. Valores típicos:
 * - `CFG_CONTROL_PERIOD_MS`: período do laço de controle em milissegundos.
 * - `CFG_IR_ALPHA`: parâmetro alpha do filtro EMA para sensores IR [0..1].
 * - `CFG_IR_TH_FREE`/`CFG_IR_TH_NEAR`: limiares de ocupação para leitura IR.
 * - `CFG_K_ROT`: ganho proporcional para centragem lateral.
 * - `CFG_FWD_BASE`: avanço base normalizado para seguimento de corredor.
 * - `CFG_TURN_FWD`/`CFG_TURN_ROT`: parâmetros de curva (avanço e rotação).
 * - `CFG_MAZE_W`/`CFG_MAZE_H`: dimensões do labirinto (células).
 * - `CFG_GOAL_X`/`CFG_GOAL_Y`: coordenadas do objetivo.
 * - `CFG_TARGET_SPEED_CM_S`: velocidade alvo (cm/s) usada para escalonamento.
 *
 * Notas:
 * - Valores fora das faixas esperadas podem ser clampados pelo código.
 * - Ajuste via CMake: `-DCFG_CONTROL_PERIOD_MS=100 -DCFG_IR_ALPHA=0.25` etc.
 */
#ifndef CFG_CONTROL_PERIOD_MS
#define CFG_CONTROL_PERIOD_MS 150
#endif
#ifndef CFG_IR_ALPHA
#define CFG_IR_ALPHA 0.23
#endif
#ifndef CFG_IR_TH_FREE
#define CFG_IR_TH_FREE 0.55
#endif
#ifndef CFG_IR_TH_NEAR
#define CFG_IR_TH_NEAR 0.30
#endif
#ifndef CFG_K_ROT
#define CFG_K_ROT 1.2
#endif
#ifndef CFG_FWD_BASE
#define CFG_FWD_BASE 0.35
#endif
#ifndef CFG_TURN_FWD
#define CFG_TURN_FWD 0.15
#endif
#ifndef CFG_TURN_ROT
#define CFG_TURN_ROT 0.7
#endif

// Configuração do labirinto (pode ser ajustada por macros futuramente)
#ifndef CFG_MAZE_W
#define CFG_MAZE_W 8
#endif
#ifndef CFG_MAZE_H
#define CFG_MAZE_H 8
#endif
#ifndef CFG_GOAL_X
#define CFG_GOAL_X 7
#endif
#ifndef CFG_GOAL_Y
#define CFG_GOAL_Y 7
#endif

/**
 * @brief Contexto compartilhado pelo callback de controle periódico.
 *
 * Armazena ponteiros para HAL e núcleo de navegação, além do estado atual
 * da pose discreta no grid (célula e heading) e um flag indicando se há
 * rota planejada.
 */
struct ControlContext {
    hal::MotorControl* motors;
    hal::IRSensorArray* sensors;
    Navigator* nav;
    // estado de navegação em grade simples
    Point cur{0,0};
    uint8_t heading{1}; // 0=N,1=E,2=S,3=W (começa para Leste)
    bool planned{false};
};

/**
 * @brief Passo de controle periódico do robô (callback do timer).
 * @param t Ponteiro para o timer que invoca o callback (user_data deve apontar
 *          para um `ControlContext`).
 * @return true para manter o timer ativo; false para parar.
 *
 * Fluxo:
 * 1) Lê sensores IR (já filtrados por EMA) e valida faixa [0..1].
 * 2) Determina flags de caminho livre com base em `CFG_IR_TH_*`.
 * 3) Atualiza mapa com paredes observadas e, se necessário, planeja rota.
 * 4) Calcula centragem lateral (erro L-R) e `rotate` via `CFG_K_ROT`.
 * 5) Calcula `forward` considerando base, velocidade alvo e proximidade frontal.
 * 6) Obtém decisão (`decide`/`decidePlanned`), loga e comanda motores via `arcadeDrive`.
 * 7) Atualiza pose discreta em avanço; salva heurísticas/mapa ao atingir o goal.
 */
static bool control_step_cb(repeating_timer_t* t) {
    auto* ctx = static_cast<ControlContext*>(t->user_data);
    // Leitura dos sensores (booleanos: caminho livre = true)
    SensorRead sr{};
    auto vals = ctx->sensors->readAll(); // valores já filtrados via EMA
    // Fail-safe: validar leituras (finito) e limitar a faixa [0,1]; se inválido, para motores e retorna
    if (!(std::isfinite(vals.left) && std::isfinite(vals.front) && std::isfinite(vals.right))) {
        ctx->motors->arcadeDrive(0.0f, 0.0f);
        return true;
    }
    auto clampf = [](float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); };
    vals.left  = clampf(vals.left,  0.0f, 1.0f);
    vals.front = clampf(vals.front, 0.0f, 1.0f);
    vals.right = clampf(vals.right, 0.0f, 1.0f);
    // Thresholds provisórios (ajustáveis por calibração): menores => mais reflexivo/perto
    const float th_free = static_cast<float>(CFG_IR_TH_FREE);   // acima disso consideramos bem livre
    const float th_near = static_cast<float>(CFG_IR_TH_NEAR);   // abaixo disso, muito perto/obstruído
    sr.left_free = vals.left  < th_free;
    sr.front_free = vals.front < th_free;
    sr.right_free = vals.right < th_free;

    // Observação de paredes no mapa usando leituras relativas
    ctx->nav->observeCellWalls(ctx->cur, sr, ctx->heading);
    if (!ctx->planned) {
        ctx->planned = ctx->nav->planRoute();
    }

    // Controle contínuo para centragem durante entradas (20cm de largura, robô 15cm)
    // Erro lateral: positivo => muito perto da esquerda (mais escuro/perto) vira à direita e vice-versa
    float err_lr = (vals.left - vals.right); // [-1..1] aprox
    // Ganho de rotação: ajuste fino; clamp para -1..1
    float k_rot = static_cast<float>(CFG_K_ROT); // base
#if CFG_AUTO_TUNE_GEOM
    // Escala k_rot pela folga lateral (menor folga => maior k_rot)
    float margin_cm = (static_cast<float>(CFG_ENTRY_WIDTH_CM) - static_cast<float>(CFG_ROBOT_WIDTH_CM)) * 0.5f;
    if (margin_cm < 1.f) margin_cm = 1.f; // evita divisão por zero e exageros
    const float ref_margin_cm = (20.0f - 15.0f) * 0.5f; // 2.5 cm de referência (defaults)
    k_rot *= (ref_margin_cm / margin_cm);
#endif
    float rotate = k_rot * err_lr;
    if (rotate > 1.f) rotate = 1.f; else if (rotate < -1.f) rotate = -1.f;

    // Velocidade base visando ~5cm/s: mapear para duty relativo. Ajuste empírico no hardware.
    float forward = static_cast<float>(CFG_FWD_BASE); // base
    // Escala a velocidade base conforme a meta de velocidade (sempre aplicado)
    const float ref_speed = 5.0f; // cm/s referência
    float scale_v = static_cast<float>(CFG_TARGET_SPEED_CM_S) / ref_speed;
    if (scale_v < 0.2f) scale_v = 0.2f; if (scale_v > 2.0f) scale_v = 2.0f; // limites razoáveis
    forward *= scale_v;
    // Reduz forward quando obstáculo à frente se aproxima
    float speed_scale = (vals.front - th_near) / (th_free - th_near); // 0..1
    if (speed_scale < 0.f) speed_scale = 0.f; else if (speed_scale > 1.f) speed_scale = 1.f;
    forward *= speed_scale;

    Decision d = ctx->planned ? ctx->nav->decidePlanned(ctx->cur, ctx->heading, sr)
                              : ctx->nav->decide(sr);

    // Log formato solicitado
    const char* lado = (d.action == Action::Right) ? "direita" :
                       (d.action == Action::Forward) ? "frente" :
                       (d.action == Action::Left) ? "esquerda" : "tras";
    printf("DECISAO lado=%s nota=%u boa=%s\n", lado, (unsigned)d.score,
           d.score >= 6 ? "sim" : "nao");

    // Motor control básico (arcade drive simplificado)
    float fwd = 0.0f, rot = 0.0f;
    // Escala de avanço para manobras de curva baseada na velocidade alvo (independente de AUTO_TUNE_GEOM)
    float turn_forward = static_cast<float>(CFG_TURN_FWD);
    {
        const float ref_speed = 5.0f; // cm/s referência
        float scale_v = static_cast<float>(CFG_TARGET_SPEED_CM_S) / ref_speed;
        if (scale_v < 0.2f) scale_v = 0.2f; if (scale_v > 2.0f) scale_v = 2.0f;
        turn_forward *= scale_v;
    }
    switch (d.action) {
        case Action::Right:
            // Clamps de segurança
            ctx->motors->arcadeDrive(clampf(turn_forward, -1.f, 1.f), clampf(static_cast<float>(+CFG_TURN_ROT), -1.f, 1.f)); // leve avanço ao entrar à direita
            // Atualiza heading (direita)
            ctx->heading = (ctx->heading + 1) & 3;
            ctx->nav->applyReward(d.action, +0.2f);
            break;
        case Action::Left:
            ctx->motors->arcadeDrive(clampf(turn_forward, -1.f, 1.f), clampf(static_cast<float>(-CFG_TURN_ROT), -1.f, 1.f));
            ctx->heading = (ctx->heading + 3) & 3;
            ctx->nav->applyReward(d.action, +0.2f);
            break;
        case Action::Back:
            ctx->motors->arcadeDrive(clampf(-0.4f, -1.f, 1.f), 0.0f);
            ctx->heading = (ctx->heading + 2) & 3;
            ctx->nav->applyReward(d.action, -0.3f); // penaliza ré
            break;
        case Action::Forward:
            // Avanço com centragem proporcional
            // Fail-safe: se obstáculo muito próximo à frente, parar
            if (vals.front <= th_near) {
                ctx->motors->arcadeDrive(0.0f, 0.0f);
                ctx->nav->applyReward(d.action, -0.2f);
            } else {
                ctx->motors->arcadeDrive(clampf(forward, -1.f, 1.f), clampf(rotate, -1.f, 1.f));
                // Atualiza célula assumindo avanço de 1 passo por iteração (modelo simplificado)
                switch (ctx->heading) {
                    case 0: if (ctx->cur.y > 0) ctx->cur.y -= 1; break;  // N
                    case 1: if (ctx->cur.x + 1 < CFG_MAZE_W) ctx->cur.x += 1; break; // E
                    case 2: if (ctx->cur.y + 1 < CFG_MAZE_H) ctx->cur.y += 1; break; // S
                    case 3: if (ctx->cur.x > 0) ctx->cur.x -= 1; break; // W
                }
                ctx->nav->applyReward(d.action, +0.3f);
                // se chegamos ao goal, persistir heurísticas e replanejar (opcional)
                if (ctx->cur.x == CFG_GOAL_X && ctx->cur.y == CFG_GOAL_Y) {
                    auto h = ctx->nav->heuristics();
                    PersistentMemory::saveHeuristics(h);
                    // Salva também o snapshot do mapa
                    PersistentMemory::saveMapSnapshot(ctx->nav->map());
                    ctx->planned = false; // permitir novo plano
                }
            }
            break;
    }
    return true; // keep repeating
}

/**
 * @brief Janela de comandos de boot via USB CDC.
 * @param window_ms Janela (ms) para ler comandos como `RESET` e `STATUS`.
 *
 * Comandos:
 * - `RESET`/`R`: apaga dados persistidos (heurísticas/mapa, conforme impl.).
 * - `STATUS`: imprime contadores/status da persistência.
 */
static void handle_boot_commands(uint32_t window_ms) {
    absolute_time_t end = make_timeout_time_ms(window_ms);
    char buf[32];
    size_t len = 0;
    printf("BOOT: aguardando comandos por %u ms (RESET/STATUS)\n", (unsigned)window_ms);
    while (!time_reached(end)) {
        int c = getchar_timeout_us(1000); // 1ms
        if (c == PICO_ERROR_TIMEOUT) continue;
        if (c == '\r') continue;
        if (c == '\n') {
            buf[len] = 0;
            if (strcmp(buf, "RESET") == 0 || strcmp(buf, "R") == 0) {
                bool ok = PersistentMemory::eraseAll();
                printf("OK RESET %s\n", ok ? "done" : "fail");
            } else if (strcmp(buf, "STATUS") == 0) {
                auto st = PersistentMemory::status();
                printf("STATUS saved=%u profile=%u\n", st.saved_count, st.active_profile);
            } else if (len) {
                printf("ERR cmd\n");
            }
            len = 0;
        } else if (len + 1 < sizeof(buf)) {
            buf[len++] = (char)c;
        } else {
            len = 0; // overflow protection
        }
    }
}

/**
 * @brief Ponto de entrada do firmware.
 *
 * Responsável por:
 * - Inicializar subsistemas (stdio/USB, LED, HAL de motores/sensores).
 * - Carregar heurísticas e snapshot do mapa, se existentes.
 * - Inicializar `Navigator` e dimensões do labirinto/objetivo.
 * - Agendar o timer periódico de controle com período `CFG_CONTROL_PERIOD_MS`.
 */
int main() {
    stdio_init_all();

    // Pequeno atraso para a USB ficar disponível antes da janela de comandos
    sleep_ms(100);

    // Janela de 3 segundos para receber RESET/STATUS
    handle_boot_commands(3000);

    // Inicialização básica de hardware
    // LED on-board (se existir): manter ligado como "alive"
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    if (LED_PIN != (uint)-1) { gpio_init(LED_PIN); gpio_set_dir(LED_PIN, GPIO_OUT); gpio_put(LED_PIN, 1); }

    // Inicializa HAL com pinos/canais vindos de macros CFG_* (definidas via CMake)
    hal::MotorControl motors(/*L_pwm=*/CFG_MOTOR_L_PWM, /*L_dirA=*/CFG_MOTOR_L_DIRA, /*L_dirB=*/CFG_MOTOR_L_DIRB,
                             /*R_pwm=*/CFG_MOTOR_R_PWM, /*R_dirA=*/CFG_MOTOR_R_DIRA, /*R_dirB=*/CFG_MOTOR_R_DIRB);
    hal::IRSensorArray sensors(/*ADC left*/CFG_IR_ADC_LEFT, /*front*/CFG_IR_ADC_FRONT, /*right*/CFG_IR_ADC_RIGHT);
    // Smoothing (EMA alpha)
    sensors.setSmoothing(static_cast<float>(CFG_IR_ALPHA));

    Navigator nav;
    nav.setStrategy(Navigator::Strategy::RightHand);
    // Mapa e objetivo
    nav.setMapDimensions(CFG_MAZE_W, CFG_MAZE_H);
    nav.setStartGoal({0,0}, {CFG_GOAL_X, CFG_GOAL_Y});
    // Carregar heurísticas persistidas, se houver
    Heuristics h{};
    if (PersistentMemory::loadHeuristics(&h)) {
        nav.setHeuristics(h);
        printf("HEUR carregadas: wr=%.2f wf=%.2f wl=%.2f wb=%.2f\n", h.w_right, h.w_front, h.w_left, h.w_back);
    } else {
        printf("HEUR padrao.\n");
    }

    // Carregar snapshot do mapa, se houver
    if (PersistentMemory::loadMapSnapshot(&nav.map())) {
        printf("MAP snapshot carregado.\n");
    } else {
        printf("MAP vazio.\n");
    }

    printf("START navegacao (timer periodico)\n");

    ControlContext ctx{ .motors = &motors, .sensors = &sensors, .nav = &nav };
    repeating_timer_t timer{};
    // Período configurável
    bool ok = add_repeating_timer_ms(CFG_CONTROL_PERIOD_MS, control_step_cb, &ctx, &timer);
    if (!ok) {
        printf("ERRO: nao foi possivel iniciar timer de controle.\n");
    }

    // Loop ocioso; o controle roda no callback do timer
    while (true) { tight_loop_contents(); }
}
