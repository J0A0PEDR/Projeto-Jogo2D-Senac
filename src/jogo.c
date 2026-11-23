#include "jogo.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define LARGURA 800
#define ALTURA 600
#define MAX_METEOROS 8
#define MAX_EXPLOSOES 15
#define MAX_HISTORICO 100
#define MAX_METEOROS_FUNDO 24
#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846

#define BLOCO_TAM 10
#define GRADE_COLS 80
#define GRADE_LINS 15
#define Y_INICIO_CIDADE (ALTURA - 70 - (GRADE_LINS * BLOCO_TAM))

typedef enum {
    MENU,
    DIFICULDADE,
    JOGANDO,
    GAME_OVER
} EstadoJogo;

typedef struct {
    float x, y;
    float velocidade;
    int num1, num2;
    char operador;
    int resultado;
    int pontos;
    bool ativo;
} Meteoro;

typedef struct {
    float x, y;
    float raio;
    int tempo_vida;
    int resultado_perdido;
    bool ativo;
} Explosao;

typedef struct {
    float x_inicio, y_inicio;
    float x_alvo, y_alvo;
    int tempo_vida;
} Laser;

typedef struct {
    char conta[32];
    int resultado;
    bool acertou;
    int pontos_ganhos;
} RegistroMeteoro;

typedef struct {
    float x, y;
    float velocidade;
    float tamanho;
    float desvio;
} MeteoroFundo;

bool blocos_cidade[GRADE_LINS][GRADE_COLS];

ALLEGRO_SAMPLE *criarSomTecla(void) {
    int amostras = (int)(SAMPLE_RATE * 0.04);
    int16_t *buf = al_malloc((size_t)amostras * sizeof(*buf));
    if (!buf) return NULL;
    for (int i = 0; i < amostras; i++) {
        double t = (double)i / SAMPLE_RATE;
        double env = 1.0 - (double)i / amostras;
        buf[i] = (int16_t)(sin(2.0 * PI * 900.0 * t) * 8000.0 * env);
    }
    ALLEGRO_SAMPLE *som = al_create_sample(buf, amostras, SAMPLE_RATE,
        ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
    if (!som) al_free(buf);
    return som;
}

ALLEGRO_SAMPLE *criarSomAcerto(void) {
    int amostras = (int)(SAMPLE_RATE * 0.22);
    int16_t *buf = al_malloc((size_t)amostras * sizeof(*buf));
    if (!buf) return NULL;
    for (int i = 0; i < amostras; i++) {
        double t = (double)i / SAMPLE_RATE;
        double freq = (i < amostras / 2) ? 587.33 : 880.0;
        double env = 1.0 - (double)i / amostras;
        double onda = sin(2.0 * PI * freq * t) > 0 ? 1.0 : -1.0;
        buf[i] = (int16_t)(onda * 7000.0 * env);
    }
    ALLEGRO_SAMPLE *som = al_create_sample(buf, amostras, SAMPLE_RATE,
        ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
    if (!som) al_free(buf);
    return som;
}

ALLEGRO_SAMPLE *criarSomExplosao(void) {
    int amostras = (int)(SAMPLE_RATE * 0.45);
    int16_t *buf = al_malloc((size_t)amostras * sizeof(*buf));
    if (!buf) return NULL;
    double ultimo = 0.0;
    for (int i = 0; i < amostras; i++) {
        double env = pow(1.0 - (double)i / amostras, 1.8);
        double ruido = (double)(rand() % 20000) / 10000.0 - 1.0;
        ultimo = ultimo * 0.85 + ruido * 0.15;
        buf[i] = (int16_t)(ultimo * 28000.0 * env);
    }
    ALLEGRO_SAMPLE *som = al_create_sample(buf, amostras, SAMPLE_RATE,
        ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
    if (!som) al_free(buf);
    return som;
}

ALLEGRO_SAMPLE *criarSomGameOver(void) {
    int amostras = (int)(SAMPLE_RATE * 0.75);
    int16_t *buf = al_malloc((size_t)amostras * sizeof(*buf));
    if (!buf) return NULL;
    for (int i = 0; i < amostras; i++) {
        double t = (double)i / SAMPLE_RATE;
        double freq = 330.0;
        if (i > amostras * 2 / 3) freq = 220.0;
        else if (i > amostras / 3) freq = 277.18;
        double env = 1.0 - (double)i / amostras * 0.7;
        double onda = sin(2.0 * PI * freq * t) > 0 ? 1.0 : -1.0;
        buf[i] = (int16_t)(onda * 8000.0 * env);
    }
    ALLEGRO_SAMPLE *som = al_create_sample(buf, amostras, SAMPLE_RATE,
        ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
    if (!som) al_free(buf);
    return som;
}

void construirCidade() {
    for (int l = 0; l < GRADE_LINS; l++) {
        for (int c = 0; c < GRADE_COLS; c++) {
            blocos_cidade[l][c] = false;
        }
    }

    int predios[][3] = {
        {4, 11, 8},
        {14, 22, 12},
        {26, 35, 7},
        {40, 48, 14},
        {52, 61, 10},
        {65, 75, 11}
    };

    for (int p = 0; p < 6; p++) {
        int col_ini = predios[p][0];
        int col_fim = predios[p][1];
        int altura_blocos = predios[p][2];

        for (int l = GRADE_LINS - altura_blocos; l < GRADE_LINS; l++) {
            for (int c = col_ini; c < col_fim; c++) {
                blocos_cidade[l][c] = true;
            }
        }
    }
}

void destruirBlocos(float exp_x, float exp_y, float raio_destruicao) {
    for (int l = 0; l < GRADE_LINS; l++) {
        for (int c = 0; c < GRADE_COLS; c++) {
            if (blocos_cidade[l][c]) {
                float bx = c * BLOCO_TAM + (BLOCO_TAM / 2.0f);
                float by = Y_INICIO_CIDADE + l * BLOCO_TAM + (BLOCO_TAM / 2.0f);

                float dx = bx - exp_x;
                float dy = by - exp_y;

                if ((dx * dx + dy * dy) <= (raio_destruicao * raio_destruicao)) {
                    blocos_cidade[l][c] = false;
                }
            }
        }
    }
}

bool verificarColisaoPredio(float mx, float my, float raio_meteoro) {
    for (int l = 0; l < GRADE_LINS; l++) {
        for (int c = 0; c < GRADE_COLS; c++) {
            if (blocos_cidade[l][c]) {
                float bx = c * BLOCO_TAM + (BLOCO_TAM / 2.0f);
                float by = Y_INICIO_CIDADE + l * BLOCO_TAM + (BLOCO_TAM / 2.0f);

                float dx = bx - mx;
                float dy = by - my;

                if ((dx * dx + dy * dy) <= ((raio_meteoro + 4.0f) * (raio_meteoro + 4.0f))) {
                    return true;
                }
            }
        }
    }
    return false;
}

void gerarConta(Meteoro *m, int nivel) {
    int op = rand() % 3;
    if (nivel == 1) {
        m->num1 = (rand() % 10) + 1;
        m->num2 = (rand() % 10) + 1;
        m->velocidade = 1.2f + ((rand() % 10) / 10.0f);
        m->pontos = 10;
    } else if (nivel == 2) {
        m->num1 = (rand() % 20) + 5;
        m->num2 = (rand() % 15) + 1;
        m->velocidade = 1.8f + ((rand() % 10) / 10.0f);
        m->pontos = 20;
    } else {
        m->num1 = (rand() % 30) + 10;
        m->num2 = (rand() % 20) + 2;
        m->velocidade = 2.4f + ((rand() % 10) / 10.0f);
        m->pontos = 30;
    }

    if (op == 0) {
        m->operador = '+';
        m->resultado = m->num1 + m->num2;
    } else if (op == 1) {
        if (m->num1 < m->num2) {
            int temp = m->num1;
            m->num1 = m->num2;
            m->num2 = temp;
        }
        m->operador = '-';
        m->resultado = m->num1 - m->num2;
    } else {
        m->num1 = (rand() % (nivel * 5)) + 2;
        m->num2 = (rand() % 10) + 1;
        m->operador = '*';
        m->resultado = m->num1 * m->num2;
    }

    m->x = 80 + (rand() % (LARGURA - 160));
    m->y = -30;
    m->ativo = true;
}

void criarExplosao(Explosao explosoes[], float x, float y, int resultado) {
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (!explosoes[i].ativo) {
            explosoes[i].x = x;
            explosoes[i].y = y;
            explosoes[i].raio = 10.0f;
            explosoes[i].tempo_vida = 60;
            explosoes[i].resultado_perdido = resultado;
            explosoes[i].ativo = true;
            break;
        }
    }
}

void registrarHistorico(RegistroMeteoro hist[], int *total, Meteoro m, bool acertou) {
    if (*total < MAX_HISTORICO) {
        snprintf(hist[*total].conta, sizeof(hist[*total].conta), "%d %c %d", m.num1, m.operador, m.num2);
        hist[*total].resultado = m.resultado;
        hist[*total].acertou = acertou;
        hist[*total].pontos_ganhos = acertou ? m.pontos : 0;
        (*total)++;
    }
}

void desenharCidadeBlocos() {
    al_draw_filled_rectangle(0, ALTURA - 70, LARGURA, ALTURA, al_map_rgb(35, 35, 45));

    for (int l = 0; l < GRADE_LINS; l++) {
        for (int c = 0; c < GRADE_COLS; c++) {
            if (blocos_cidade[l][c]) {
                float x1 = c * BLOCO_TAM;
                float y1 = Y_INICIO_CIDADE + l * BLOCO_TAM;

                al_draw_filled_rectangle(x1, y1, x1 + BLOCO_TAM, y1 + BLOCO_TAM, al_map_rgb(60, 75, 100));
                al_draw_rectangle(x1, y1, x1 + BLOCO_TAM, y1 + BLOCO_TAM, al_map_rgb(40, 50, 70), 1);
            }
        }
    }
}

// Predios distantes e meteoros pequenos sao apenas elementos visuais.
// Nao alteram a colisao nem a vida da cidade.
void criarMeteoroFundo(MeteoroFundo *m, bool no_topo) {
    m->x = (float)(rand() % LARGURA);
    m->y = no_topo ? (float)(-15 - rand() % 250) : (float)(rand() % 420);
    m->velocidade = 0.55f + (float)(rand() % 90) / 100.0f;
    m->tamanho = 1.5f + (float)(rand() % 20) / 10.0f;
    m->desvio = 0.12f + (float)(rand() % 30) / 100.0f;
}

void desenharFundoPartida(const MeteoroFundo meteoros_fundo[], int quadros) {
    // Ceu noturno em faixas suaves.
    al_draw_filled_rectangle(0, 0, LARGURA, 175, al_map_rgb(10, 17, 38));
    al_draw_filled_rectangle(0, 175, LARGURA, 330, al_map_rgb(15, 24, 48));
    al_draw_filled_rectangle(0, 330, LARGURA, 460, al_map_rgb(22, 34, 57));

    // Estrelas discretas, sem atrapalhar as contas dos meteoros.
    for (int i = 0; i < 42; i++) {
        float x = (float)((i * 173 + 47) % LARGURA);
        float y = (float)((i * 89 + 13) % 365);
        int brilho = 75 + ((quadros / 24 + i) % 3) * 15;
        al_draw_filled_circle(x, y, 0.8f,
            al_map_rgb(brilho, brilho + 8, brilho + 22));
    }

    // Primeira camada de edificios, mais escura e distante.
    for (int i = 0; i < 21; i++) {
        float x = (float)(i * 40 - 5);
        float topo = 425.0f - (float)((i * 41 + 22) % 100);
        al_draw_filled_rectangle(x, topo, x + 35, 530,
            al_map_rgb(25, 37, 60));
    }

    // Segunda camada com algumas janelas iluminadas.
    for (int i = 0; i < 15; i++) {
        float x = (float)(i * 56 - 7);
        float topo = 480.0f - (float)((i * 53 + 17) % 95);
        al_draw_filled_rectangle(x, topo, x + 48, 535,
            al_map_rgb(34, 47, 70));
        for (int linha = 0; linha < 5; linha++) {
            float y = topo + 15.0f + linha * 19.0f;
            if (y >= 520) break;
            for (int coluna = 0; coluna < 3; coluna++) {
                if ((i + linha * 2 + coluna * 3) % 4 == 0) continue;
                float wx = x + 9.0f + coluna * 13.0f;
                al_draw_filled_rectangle(wx, y, wx + 5, y + 8,
                    al_map_rgb(94, 112, 130));
            }
        }
    }

    // Rastros pequenos, atras dos meteoros com contas.
    for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
        const MeteoroFundo *m = &meteoros_fundo[i];
        if (m->y < -15 || m->y > 475) continue;
        al_draw_line(m->x - m->desvio * 28.0f,
            m->y - m->velocidade * 13.0f,
            m->x, m->y, al_map_rgb(119, 62, 37), 1.4f);
        al_draw_filled_circle(m->x, m->y, m->tamanho,
            al_map_rgb(232, 142, 75));
        al_draw_filled_circle(m->x - 0.4f, m->y - 0.4f,
            m->tamanho * 0.4f, al_map_rgb(255, 205, 122));
    }
}

// Elementos visuais do menu feitos diretamente com Allegro.
void desenharTextoGrande(ALLEGRO_FONT *fonte, const char *texto,
                        float x, float y, float escala, ALLEGRO_COLOR cor) {
    ALLEGRO_TRANSFORM anterior = *al_get_current_transform();
    ALLEGRO_TRANSFORM atual;
    al_identity_transform(&atual);
    al_scale_transform(&atual, escala, escala);
    al_translate_transform(&atual, x, y);
    al_use_transform(&atual);
    al_draw_text(fonte, cor, 0, 0, ALLEGRO_ALIGN_CENTER, texto);
    al_use_transform(&anterior);
}

void desenharFundoMenu(int quadros) {
    al_clear_to_color(al_map_rgb(8, 12, 29));

    // Estrelas fixas com cintilacao discreta.
    for (int i = 0; i < 65; i++) {
        float x = (float)((i * 137 + 23) % LARGURA);
        float y = (float)((i * 71 + 17) % 460);
        int brilho = 90 + ((quadros / 13 + i * 7) % 4) * 36;
        al_draw_filled_circle(x, y, i % 9 == 0 ? 1.8f : 1.0f,
                              al_map_rgb(brilho / 2, brilho / 2 + 12, brilho));
    }

    // Rastro de meteoro ao fundo.
    float mx = (float)((quadros * 2) % 1050) - 125.0f;
    float my = 70.0f + (mx + 125.0f) * 0.27f;
    al_draw_line(mx - 76, my - 34, mx, my, al_map_rgb(155, 50, 25), 9);
    al_draw_line(mx - 50, my - 22, mx, my, al_map_rgb(255, 120, 35), 5);
    al_draw_filled_circle(mx, my, 11, al_map_rgb(255, 102, 35));
    al_draw_filled_circle(mx - 2, my - 2, 5, al_map_rgb(255, 211, 115));

    // Silhueta da cidade na parte inferior.
    al_draw_filled_rectangle(0, 555, LARGURA, ALTURA, al_map_rgb(15, 23, 43));
    for (int i = 0; i < 17; i++) {
        float x = (float)(i * 49);
        float altura = (float)(42 + (i * 37) % 85);
        float y = 555 - altura;
        al_draw_filled_rectangle(x, y, x + 44, 558,
                                 al_map_rgb(21, 31, 54));
        for (int j = 0; j < 3; j++) {
            float wy = y + 12 + j * 20;
            if (wy < 545) {
                al_draw_filled_rectangle(x + 10, wy, x + 15, wy + 8,
                                         al_map_rgb(75, 105, 135));
                al_draw_filled_rectangle(x + 29, wy, x + 34, wy + 8,
                                         al_map_rgb(75, 105, 135));
            }
        }
    }
    al_draw_line(0, 557, LARGURA, 557, al_map_rgb(32, 105, 138), 2);
}

void desenharBotaoMenu(ALLEGRO_FONT *fonte, float x1, float y1, float x2,
                       float y2, const char *texto, bool destaque,
                       ALLEGRO_COLOR cor) {
    ALLEGRO_COLOR fundo = destaque ? al_map_rgb(32, 174, 187) : cor;
    al_draw_filled_rounded_rectangle(x1 + 2, y1 + 6, x2 + 2, y2 + 6,
                                     9, 9, al_map_rgb(5, 10, 22));
    al_draw_filled_rounded_rectangle(x1, y1, x2, y2, 9, 9, fundo);
    al_draw_rounded_rectangle(x1, y1, x2, y2, 9, 9,
                              destaque ? al_map_rgb(173, 255, 255) : al_map_rgb(92, 124, 154), 2);
    desenharTextoGrande(fonte, texto, (x1 + x2) / 2, (y1 + y2) / 2 - 11,
                        1.8f, al_map_rgb(255, 255, 255));
}


const char *obterClassificacao(int pontuacao, float precisao) {
    if (pontuacao >= 400 && precisao >= 85.0f) return "MESTRE";
    if (pontuacao >= 250 && precisao >= 70.0f) return "EXCELENTE";
    if (pontuacao >= 120 && precisao >= 50.0f) return "BOM";
    return "INICIANTE";
}

ALLEGRO_COLOR corClassificacao(const char *classificacao) {
    if (strcmp(classificacao, "MESTRE") == 0) return al_map_rgb(255, 209, 72);
    if (strcmp(classificacao, "EXCELENTE") == 0) return al_map_rgb(92, 245, 185);
    if (strcmp(classificacao, "BOM") == 0) return al_map_rgb(92, 185, 255);
    return al_map_rgb(183, 195, 217);
}

void desenharCartaoResultado(ALLEGRO_FONT *fonte,
                            float x, float y, float largura,
                            const char *rotulo, const char *valor,
                            ALLEGRO_COLOR cor) {
    al_draw_filled_rounded_rectangle(
        x, y, x + largura, y + 62, 8, 8, al_map_rgb(22, 35, 55));
    al_draw_rounded_rectangle(
        x, y, x + largura, y + 62, 8, 8, al_map_rgb(48, 76, 99), 1);
    al_draw_filled_rectangle(x + 9, y + 12, x + 12, y + 50, cor);
    al_draw_text(fonte, al_map_rgb(174, 191, 211),
                 x + largura / 2, y + 11, ALLEGRO_ALIGN_CENTER, rotulo);
    desenharTextoGrande(fonte, valor, x + largura / 2, y + 31,
                       1.65f, cor);
}

int executar_jogo(void) {
    srand((unsigned int)time(NULL));

    al_init();
    al_init_font_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();

    bool audio_ok = al_install_audio();
    if (audio_ok) audio_ok = al_reserve_samples(8);
    ALLEGRO_SAMPLE *som_tecla = audio_ok ? criarSomTecla() : NULL;
    ALLEGRO_SAMPLE *som_acerto = audio_ok ? criarSomAcerto() : NULL;
    ALLEGRO_SAMPLE *som_explosao = audio_ok ? criarSomExplosao() : NULL;
    ALLEGRO_SAMPLE *som_gameover = audio_ok ? criarSomGameOver() : NULL;

    ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE *fila = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_FONT *fonte = al_create_builtin_font();

    al_register_event_source(fila, al_get_display_event_source(display));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_mouse_event_source());

    EstadoJogo estado = MENU;
    int nivel = 1;
    int vida_cidade = 100;
    int pontos_totais = 0;
    int acertos = 0;
    int erros = 0;
    int timer_spawn = 0;

    char input_texto[16] = "";
    int input_pos = 0;

    Meteoro meteoros[MAX_METEOROS];
    Explosao explosoes[MAX_EXPLOSOES];
    Laser laser = { 0 };
    MeteoroFundo meteoros_fundo[MAX_METEOROS_FUNDO];
    RegistroMeteoro historico[MAX_HISTORICO];
    int total_historico = 0;
    int scroll_offset = 0;

    for (int i = 0; i < MAX_METEOROS; i++) meteoros[i].ativo = false;
    for (int i = 0; i < MAX_EXPLOSOES; i++) explosoes[i].ativo = false;
    for (int i = 0; i < MAX_METEOROS_FUNDO; i++)
        criarMeteoroFundo(&meteoros_fundo[i], false);

    al_start_timer(timer);
    bool rodando = true;
    bool redesenhar = true;
    int quadros_menu = 0;
    int mouse_x = -1, mouse_y = -1;

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mouse_x = ev.mouse.x;
            mouse_y = ev.mouse.y;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            mouse_x = ev.mouse.x;
            mouse_y = ev.mouse.y;
            int mx = ev.mouse.x;
            int my = ev.mouse.y;

            if (estado == MENU) {
                if (mx >= 285 && mx <= 515 && my >= 315 && my <= 375) {
                    if (som_tecla) al_play_sample(som_tecla, 0.6f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
                    estado = DIFICULDADE;
                }
            } else if (estado == DIFICULDADE) {
                bool iniciou = false;
                if (mx >= 300 && mx <= 500 && my >= 200 && my <= 250) { nivel = 1; iniciou = true; }
                if (mx >= 300 && mx <= 500 && my >= 280 && my <= 330) { nivel = 2; iniciou = true; }
                if (mx >= 300 && mx <= 500 && my >= 360 && my <= 410) { nivel = 3; iniciou = true; }

                if (iniciou) {
                    if (som_tecla) al_play_sample(som_tecla, 0.6f, 0.0f, 1.2f, ALLEGRO_PLAYMODE_ONCE, NULL);
                    construirCidade();
                    vida_cidade = 100;
                    pontos_totais = 0;
                    acertos = 0;
                    erros = 0;
                    total_historico = 0;
                    scroll_offset = 0;
                    timer_spawn = 0;
                    laser.tempo_vida = 0;
                    input_pos = 0;
                    input_texto[0] = '\0';
                    for (int i = 0; i < MAX_METEOROS; i++) meteoros[i].ativo = false;
                    for (int i = 0; i < MAX_EXPLOSOES; i++) explosoes[i].ativo = false;
                    estado = JOGANDO;
                }
            } else if (estado == GAME_OVER) {
                if (mx >= 285 && mx <= 515 && my >= 515 && my <= 562) {
                    if (som_tecla) al_play_sample(som_tecla, 0.6f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
                    estado = MENU;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (estado == JOGANDO) {
                if (ev.keyboard.unichar >= '0' && ev.keyboard.unichar <= '9' && input_pos < 10) {
                    input_texto[input_pos++] = (char)ev.keyboard.unichar;
                    input_texto[input_pos] = '\0';
                    if (som_tecla) al_play_sample(som_tecla, 0.4f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && input_pos > 0) {
                    input_texto[--input_pos] = '\0';
                    if (som_tecla) al_play_sample(som_tecla, 0.4f, 0.0f, 0.8f, ALLEGRO_PLAYMODE_ONCE, NULL);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER && input_pos > 0) {
                    int resposta = atoi(input_texto);
                    int alvo = -1;
                    float maior_y = -1000.0f;

                    for (int i = 0; i < MAX_METEOROS; i++) {
                        if (meteoros[i].ativo && meteoros[i].resultado == resposta) {
                            if (meteoros[i].y > maior_y) {
                                maior_y = meteoros[i].y;
                                alvo = i;
                            }
                        }
                    }

                    if (alvo != -1) {
                        laser.x_inicio = LARGURA / 2.0f;
                        laser.y_inicio = ALTURA - 70.0f;
                        laser.x_alvo = meteoros[alvo].x;
                        laser.y_alvo = meteoros[alvo].y;
                        laser.tempo_vida = 12;

                        pontos_totais += meteoros[alvo].pontos;
                        acertos++;
                        registrarHistorico(historico, &total_historico, meteoros[alvo], true);
                        meteoros[alvo].ativo = false;
                        if (som_acerto) al_play_sample(som_acerto, 0.9f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }

                    input_pos = 0;
                    input_texto[0] = '\0';
                }
            } else if (estado == GAME_OVER) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && scroll_offset + 5 < total_historico) {
                    scroll_offset++;
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_UP && scroll_offset > 0) {
                    scroll_offset--;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            redesenhar = true;
            quadros_menu++;

            if (laser.tempo_vida > 0) {
                laser.tempo_vida--;
            }

            if (estado == JOGANDO) {
                for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
                    meteoros_fundo[i].x += meteoros_fundo[i].desvio;
                    meteoros_fundo[i].y += meteoros_fundo[i].velocidade;
                    if (meteoros_fundo[i].y > 475 || meteoros_fundo[i].x > LARGURA + 15)
                        criarMeteoroFundo(&meteoros_fundo[i], true);
                }

                timer_spawn++;
                int intervalo = (nivel == 1) ? 120 : (nivel == 2) ? 95 : 75;

                if (timer_spawn >= intervalo) {
                    timer_spawn = 0;
                    for (int i = 0; i < MAX_METEOROS; i++) {
                        if (!meteoros[i].ativo) {
                            gerarConta(&meteoros[i], nivel);
                            break;
                        }
                    }
                }

                for (int i = 0; i < MAX_METEOROS; i++) {
                    if (meteoros[i].ativo) {
                        meteoros[i].y += meteoros[i].velocidade;

                        bool bateu_predio = verificarColisaoPredio(meteoros[i].x, meteoros[i].y, 26.0f);
                        bool bateu_chao = (meteoros[i].y >= ALTURA - 75);

                        if (bateu_predio || bateu_chao) {
                            destruirBlocos(meteoros[i].x, meteoros[i].y, 45.0f);
                            criarExplosao(explosoes, meteoros[i].x, meteoros[i].y, meteoros[i].resultado);
                            registrarHistorico(historico, &total_historico, meteoros[i], false);
                            meteoros[i].ativo = false;
                            vida_cidade -= 20;
                            erros++;
                            if (som_explosao) al_play_sample(som_explosao, 0.9f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);

                            if (vida_cidade <= 0) {
                                vida_cidade = 0;
                                estado = GAME_OVER;
                                if (som_gameover) al_play_sample(som_gameover, 0.9f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
                            }
                        }
                    }
                }

                for (int i = 0; i < MAX_EXPLOSOES; i++) {
                    if (explosoes[i].ativo) {
                        explosoes[i].raio += 1.2f;
                        explosoes[i].tempo_vida--;
                        if (explosoes[i].tempo_vida <= 0) {
                            explosoes[i].ativo = false;
                        }
                    }
                }
            }
        }

        if (redesenhar && al_is_event_queue_empty(fila)) {
            redesenhar = false;
            al_clear_to_color(al_map_rgb(15, 18, 30));

            if (estado == MENU) {
                desenharFundoMenu(quadros_menu);

                al_draw_filled_rounded_rectangle(177, 106, 623, 467,
                    16, 16, al_map_rgb(14, 23, 44));
                al_draw_rounded_rectangle(177, 106, 623, 467,
                    16, 16, al_map_rgb(44, 130, 165), 2);
                al_draw_line(221, 129, 579, 129, al_map_rgb(54, 87, 118), 2);

                desenharTextoGrande(fonte, "NUMBER", 400, 157, 4.0f,
                                    al_map_rgb(255, 211, 89));
                desenharTextoGrande(fonte, "STRIKER", 400, 206, 4.0f,
                                    al_map_rgb(78, 226, 255));
                al_draw_text(fonte, al_map_rgb(175, 191, 213), 400, 264,
                             ALLEGRO_ALIGN_CENTER, "DEFESA MATEMATICA");
                al_draw_text(fonte, al_map_rgb(205, 216, 230), 400, 286,
                             ALLEGRO_ALIGN_CENTER, "Resolva contas e proteja a cidade!");

                bool sobre = mouse_x >= 285 && mouse_x <= 515 &&
                             mouse_y >= 315 && mouse_y <= 375;
                desenharBotaoMenu(fonte, 285, 315, 515, 375, "JOGAR",
                                 sobre, al_map_rgb(20, 115, 143));
                al_draw_text(fonte, al_map_rgb(133, 171, 192), 400, 409,
                             ALLEGRO_ALIGN_CENTER, "3 NIVEIS  |  METEOROS  |  LASER");
                al_draw_text(fonte, al_map_rgb(120, 138, 165), 400, 439,
                             ALLEGRO_ALIGN_CENTER, "USE O MOUSE PARA COMECAR");
            }
            else if (estado == DIFICULDADE) {
                desenharFundoMenu(quadros_menu);
                al_draw_filled_rounded_rectangle(184, 102, 616, 469,
                    15, 15, al_map_rgb(14, 23, 44));
                al_draw_rounded_rectangle(184, 102, 616, 469,
                    15, 15, al_map_rgb(44, 130, 165), 2);
                desenharTextoGrande(fonte, "DIFICULDADE", 400, 129, 2.5f,
                                    al_map_rgb(255, 213, 108));
                al_draw_text(fonte, al_map_rgb(175, 191, 213), 400, 172,
                             ALLEGRO_ALIGN_CENTER, "Escolha o desafio e defenda sua cidade");

                desenharBotaoMenu(fonte, 300, 200, 500, 250, "FACIL",
                    mouse_x >= 300 && mouse_x <= 500 && mouse_y >= 200 && mouse_y <= 250,
                    al_map_rgb(30, 117, 83));
                desenharBotaoMenu(fonte, 300, 280, 500, 330, "MEDIO",
                    mouse_x >= 300 && mouse_x <= 500 && mouse_y >= 280 && mouse_y <= 330,
                    al_map_rgb(148, 108, 30));
                desenharBotaoMenu(fonte, 300, 360, 500, 410, "DIFICIL",
                    mouse_x >= 300 && mouse_x <= 500 && mouse_y >= 360 && mouse_y <= 410,
                    al_map_rgb(156, 55, 68));
                al_draw_text(fonte, al_map_rgb(132, 157, 184), 400, 440,
                             ALLEGRO_ALIGN_CENTER, "Escolha um nivel para iniciar");
            }
            else if (estado == JOGANDO) {
                desenharFundoPartida(meteoros_fundo, quadros_menu);
                desenharCidadeBlocos();

                // Origem do disparo, na parte central da cidade.
                al_draw_filled_triangle(
                    LARGURA / 2.0f - 14, ALTURA - 65.0f,
                    LARGURA / 2.0f + 14, ALTURA - 65.0f,
                    LARGURA / 2.0f, ALTURA - 85.0f,
                    al_map_rgb(30, 190, 255));

                for (int i = 0; i < MAX_METEOROS; i++) {
                    if (meteoros[i].ativo) {
                        al_draw_filled_circle(meteoros[i].x, meteoros[i].y, 26, al_map_rgb(160, 75, 40));
                        al_draw_circle(meteoros[i].x, meteoros[i].y, 26, al_map_rgb(255, 140, 50), 2);

                        char texto_conta[32];
                        snprintf(texto_conta, sizeof(texto_conta), "%d %c %d", meteoros[i].num1, meteoros[i].operador, meteoros[i].num2);
                        al_draw_text(fonte, al_map_rgb(255, 255, 255), meteoros[i].x, meteoros[i].y - 4, ALLEGRO_ALIGN_CENTER, texto_conta);
                    }
                }

                // O feixe aponta para a posicao do meteoro atingido.
                if (laser.tempo_vida > 0) {
                    float intensidade = laser.tempo_vida / 12.0f;
                    al_draw_line(laser.x_inicio, laser.y_inicio,
                        laser.x_alvo, laser.y_alvo,
                        al_map_rgba_f(0.0f, 0.8f, 1.0f, intensidade), 7.0f);
                    al_draw_line(laser.x_inicio, laser.y_inicio,
                        laser.x_alvo, laser.y_alvo,
                        al_map_rgba_f(1.0f, 1.0f, 1.0f, intensidade), 2.5f);
                    al_draw_filled_circle(laser.x_alvo, laser.y_alvo,
                        8.0f + (12 - laser.tempo_vida) * 1.2f,
                        al_map_rgba_f(0.3f, 0.95f, 1.0f, intensidade));
                }

                for (int i = 0; i < MAX_EXPLOSOES; i++) {
                    if (explosoes[i].ativo) {
                        al_draw_filled_circle(explosoes[i].x, explosoes[i].y, explosoes[i].raio, al_map_rgb(255, 80, 20));
                        al_draw_filled_circle(explosoes[i].x, explosoes[i].y, explosoes[i].raio * 0.6f, al_map_rgb(255, 210, 50));

                        char texto_res[32];
                        snprintf(texto_res, sizeof(texto_res), "Resp: %d", explosoes[i].resultado_perdido);
                        al_draw_text(fonte, al_map_rgb(255, 255, 255), explosoes[i].x, explosoes[i].y - explosoes[i].raio - 15, ALLEGRO_ALIGN_CENTER, texto_res);
                    }
                }

                al_draw_textf(fonte, al_map_rgb(255, 255, 255), 20, 20, 0, "VIDA CIDADE: %d%%", vida_cidade);
                al_draw_textf(fonte, al_map_rgb(255, 215, 0), 20, 40, 0, "PONTOS: %d", pontos_totais);

                al_draw_filled_rectangle(300, ALTURA - 45, 500, ALTURA - 15, al_map_rgb(20, 20, 30));
                al_draw_rectangle(300, ALTURA - 45, 500, ALTURA - 15, al_map_rgb(100, 200, 255), 2);
                al_draw_textf(fonte, al_map_rgb(100, 255, 150), LARGURA / 2, ALTURA - 34, ALLEGRO_ALIGN_CENTER, "DIGITE: %s", input_texto);
            }
            else if (estado == GAME_OVER) {
                int total_meteoros = acertos + erros;
                float precisao = total_meteoros > 0 ?
                    100.0f * (float)acertos / (float)total_meteoros : 0.0f;
                const char *classificacao = obterClassificacao(pontos_totais, precisao);
                ALLEGRO_COLOR cor_rank = corClassificacao(classificacao);

                char texto_pontos[24], texto_acertos[20], texto_erros[20];
                char texto_precisao[24], texto_vida[20];
                snprintf(texto_pontos, sizeof texto_pontos, "%d", pontos_totais);
                snprintf(texto_acertos, sizeof texto_acertos, "%d", acertos);
                snprintf(texto_erros, sizeof texto_erros, "%d", erros);
                snprintf(texto_precisao, sizeof texto_precisao, "%.1f%%", precisao);
                snprintf(texto_vida, sizeof texto_vida, "%d%%", vida_cidade);

                desenharFundoMenu(quadros_menu);
                al_draw_filled_rounded_rectangle(68, 24, 732, 577,
                    15, 15, al_map_rgb(12, 22, 40));
                al_draw_rounded_rectangle(68, 24, 732, 577,
                    15, 15, al_map_rgb(53, 145, 177), 2);
                al_draw_line(94, 69, 706, 69, al_map_rgb(54, 92, 120), 1);

                desenharTextoGrande(fonte, "FIM DE JOGO", 400, 37, 2.7f,
                    al_map_rgb(255, 108, 112));
                al_draw_text(fonte, al_map_rgb(172, 194, 216), 400, 77,
                    ALLEGRO_ALIGN_CENTER, "RESUMO DA SUA PARTIDA");

                al_draw_filled_rounded_rectangle(266, 103, 534, 167,
                    10, 10, al_map_rgb(27, 39, 59));
                al_draw_rounded_rectangle(266, 103, 534, 167,
                    10, 10, al_map_rgb(190, 151, 65), 2);
                al_draw_text(fonte, al_map_rgb(241, 201, 100), 400, 112,
                    ALLEGRO_ALIGN_CENTER, "PONTUACAO FINAL");
                desenharTextoGrande(fonte, texto_pontos, 400, 135, 2.5f,
                    al_map_rgb(255, 231, 160));

                al_draw_text(fonte, al_map_rgb(172, 194, 216), 400, 176,
                    ALLEGRO_ALIGN_CENTER, "CLASSIFICACAO");
                desenharTextoGrande(fonte, classificacao, 400, 191,
                    1.9f, cor_rank);

                desenharCartaoResultado(fonte, 92, 229, 145,
                    "ACERTOS", texto_acertos, al_map_rgb(92, 237, 161));
                desenharCartaoResultado(fonte, 249, 229, 145,
                    "QUEDAS", texto_erros, al_map_rgb(255, 116, 105));
                desenharCartaoResultado(fonte, 406, 229, 145,
                    "PRECISAO", texto_precisao, al_map_rgb(104, 199, 255));
                desenharCartaoResultado(fonte, 563, 229, 145,
                    "VIDA CIDADE", texto_vida, al_map_rgb(245, 195, 101));

                // Barra de precisao: de 0 a 100%.
                al_draw_filled_rounded_rectangle(94, 304, 706, 315,
                    5, 5, al_map_rgb(35, 48, 70));
                if (precisao > 0.0f) {
                    al_draw_filled_rounded_rectangle(94, 304,
                        94 + (612.0f * precisao / 100.0f), 315,
                        5, 5, al_map_rgb(73, 205, 190));
                }

                al_draw_text(fonte, al_map_rgb(222, 231, 245), 95, 325, 0,
                    "HISTORICO DE METEOROS");
                al_draw_text(fonte, al_map_rgb(154, 179, 203), 695, 325,
                    ALLEGRO_ALIGN_RIGHT, "ULTIMOS REGISTROS");
                al_draw_line(93, 345, 707, 345, al_map_rgb(65, 91, 115), 1);

                int visiveis = 5;
                int inicio = scroll_offset;
                for (int i = 0; i < visiveis && inicio + i < total_historico; i++) {
                    int idx = inicio + i;
                    float y = 358.0f + i * 24.0f;
                    ALLEGRO_COLOR cor = historico[idx].acertou ?
                        al_map_rgb(90, 234, 157) : al_map_rgb(255, 128, 112);
                    if (i % 2 == 0) {
                        al_draw_filled_rectangle(94, y - 3, 706, y + 19,
                            al_map_rgb(20, 33, 51));
                    }
                    al_draw_text(fonte, al_map_rgb(231, 238, 248), 110, y, 0,
                        historico[idx].conta);
                    al_draw_textf(fonte, al_map_rgb(196, 214, 231), 280, y,
                        0, "= %d", historico[idx].resultado);
                    al_draw_text(fonte, cor, 442, y, 0,
                        historico[idx].acertou ? "ACERTOU" : "CAIU");
                    al_draw_textf(fonte, cor, 596, y, 0,
                        "+%d pts", historico[idx].pontos_ganhos);
                }
                if (total_historico == 0) {
                    al_draw_text(fonte, al_map_rgb(152, 173, 195), 400, 405,
                        ALLEGRO_ALIGN_CENTER, "Nenhuma conta registrada.");
                }

                al_draw_text(fonte, al_map_rgb(147, 174, 195), 400, 485,
                    ALLEGRO_ALIGN_CENTER,
                    total_historico > 5 ?
                    "SETAS CIMA / BAIXO: VER MAIS RESULTADOS" :
                    "RESOLVA MAIS CONTAS NA PROXIMA PARTIDA");

                desenharBotaoMenu(fonte, 285, 515, 515, 562,
                    "VOLTAR AO MENU",
                    mouse_x >= 285 && mouse_x <= 515 &&
                    mouse_y >= 515 && mouse_y <= 562,
                    al_map_rgb(24, 116, 151));
            }

            al_flip_display();
        }
    }

    if (som_tecla) al_destroy_sample(som_tecla);
    if (som_acerto) al_destroy_sample(som_acerto);
    if (som_explosao) al_destroy_sample(som_explosao);
    if (som_gameover) al_destroy_sample(som_gameover);
    if (al_is_audio_installed()) al_uninstall_audio();

    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(display);

    return 0;
}
