#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
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

#define BLOCO_TAM 10
#define GRADE_COLS 80
#define GRADE_LINS 15
#define Y_INICIO_CIDADE (ALTURA - 70 - (GRADE_LINS * BLOCO_TAM))

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846

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
    char conta[32];
    int resultado;
    bool acertou;
    int pontos_ganhos;
} RegistroMeteoro;

bool blocos_cidade[GRADE_LINS][GRADE_COLS];

ALLEGRO_SAMPLE* criarSomTecla() {
    int amostras = (int)(SAMPLE_RATE * 0.04);
    int16_t* buf = (int16_t*)al_malloc(amostras * sizeof(int16_t));
    for (int i = 0; i < amostras; i++) {
        double t = (double)i / SAMPLE_RATE;
        double env = 1.0 - ((double)i / amostras);
        buf[i] = (int16_t)(sin(2.0 * PI * 900.0 * t) * 8000.0 * env);
    }
    return al_create_sample(buf, amostras, SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
}

ALLEGRO_SAMPLE* criarSomAcerto() {
    int amostras = (int)(SAMPLE_RATE * 0.22);
    int16_t* buf = (int16_t*)al_malloc(amostras * sizeof(int16_t));
    for (int i = 0; i < amostras; i++) {
        double t = (double)i / SAMPLE_RATE;
        double freq = (i < amostras / 2) ? 587.33 : 880.0;
        double env = 1.0 - ((double)i / amostras);
        double onda = sin(2.0 * PI * freq * t) > 0 ? 1.0 : -1.0;
        buf[i] = (int16_t)(onda * 7000.0 * env);
    }
    return al_create_sample(buf, amostras, SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
}

ALLEGRO_SAMPLE* criarSomExplosao() {
    int amostras = (int)(SAMPLE_RATE * 0.45);
    int16_t* buf = (int16_t*)al_malloc(amostras * sizeof(int16_t));
    double ultimo = 0.0;
    for (int i = 0; i < amostras; i++) {
        double env = pow(1.0 - ((double)i / amostras), 1.8);
        double ruido = ((double)(rand() % 20000) / 10000.0) - 1.0;
        ultimo = (ultimo * 0.85) + (ruido * 0.15);
        buf[i] = (int16_t)(ultimo * 28000.0 * env);
    }
    return al_create_sample(buf, amostras, SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
}

ALLEGRO_SAMPLE* criarSomGameOver() {
    int amostras = (int)(SAMPLE_RATE * 0.75);
    int16_t* buf = (int16_t*)al_malloc(amostras * sizeof(int16_t));
    for (int i = 0; i < amostras; i++) {
        double t = (double)i / SAMPLE_RATE;
        double freq = 330.0;
        if (i > amostras * 2 / 3) freq = 220.0;
        else if (i > amostras / 3) freq = 277.18;
        double env = 1.0 - ((double)i / amostras) * 0.7;
        double onda = sin(2.0 * PI * freq * t) > 0 ? 1.0 : -1.0;
        buf[i] = (int16_t)(onda * 8000.0 * env);
    }
    return al_create_sample(buf, amostras, SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1, true);
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
        sprintf_s(hist[*total].conta, sizeof(hist[*total].conta), "%d %c %d", m.num1, m.operador, m.num2);
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

int main() {
    srand((unsigned int)time(NULL));

    al_init();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_install_keyboard();
    al_install_mouse();

    bool audio_ok = al_install_audio();
    if (audio_ok) {
        al_reserve_samples(8);
    }

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
    RegistroMeteoro historico[MAX_HISTORICO];
    int total_historico = 0;
    int scroll_offset = 0;

    for (int i = 0; i < MAX_METEOROS; i++) meteoros[i].ativo = false;
    for (int i = 0; i < MAX_EXPLOSOES; i++) explosoes[i].ativo = false;

    al_start_timer(timer);
    bool rodando = true;
    bool redesenhar = true;

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = ev.mouse.x;
            int my = ev.mouse.y;

            if (estado == MENU) {
                if (mx >= 300 && mx <= 500 && my >= 260 && my <= 320) {
                    if (som_tecla) al_play_sample(som_tecla, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    estado = DIFICULDADE;
                }
            } else if (estado == DIFICULDADE) {
                bool iniciou = false;
                if (mx >= 300 && mx <= 500 && my >= 200 && my <= 250) { nivel = 1; iniciou = true; }
                if (mx >= 300 && mx <= 500 && my >= 280 && my <= 330) { nivel = 2; iniciou = true; }
                if (mx >= 300 && mx <= 500 && my >= 360 && my <= 410) { nivel = 3; iniciou = true; }

                if (iniciou) {
                    if (som_tecla) al_play_sample(som_tecla, 1.0, 0.0, 1.2, ALLEGRO_PLAYMODE_ONCE, NULL);
                    construirCidade();
                    vida_cidade = 100;
                    pontos_totais = 0;
                    acertos = 0;
                    erros = 0;
                    total_historico = 0;
                    scroll_offset = 0;
                    input_pos = 0;
                    input_texto[0] = '\0';
                    for (int i = 0; i < MAX_METEOROS; i++) meteoros[i].ativo = false;
                    for (int i = 0; i < MAX_EXPLOSOES; i++) explosoes[i].ativo = false;
                    estado = JOGANDO;
                }
            } else if (estado == GAME_OVER) {
                if (mx >= 300 && mx <= 500 && my >= 520 && my <= 570) {
                    if (som_tecla) al_play_sample(som_tecla, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    estado = MENU;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (estado == JOGANDO) {
                if (ev.keyboard.unichar >= '0' && ev.keyboard.unichar <= '9' && input_pos < 10) {
                    input_texto[input_pos++] = (char)ev.keyboard.unichar;
                    input_texto[input_pos] = '\0';
                    if (som_tecla) al_play_sample(som_tecla, 0.7, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && input_pos > 0) {
                    input_texto[--input_pos] = '\0';
                    if (som_tecla) al_play_sample(som_tecla, 0.7, 0.0, 0.8, ALLEGRO_PLAYMODE_ONCE, NULL);
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
                        pontos_totais += meteoros[alvo].pontos;
                        acertos++;
                        registrarHistorico(historico, &total_historico, meteoros[alvo], true);
                        meteoros[alvo].ativo = false;
                        if (som_acerto) al_play_sample(som_acerto, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                    }

                    input_pos = 0;
                    input_texto[0] = '\0';
                }
            } else if (estado == GAME_OVER) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN && scroll_offset + 12 < total_historico) {
                    scroll_offset++;
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_UP && scroll_offset > 0) {
                    scroll_offset--;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            redesenhar = true;

            if (estado == JOGANDO) {
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

                            if (vida_cidade <= 0) {
                                vida_cidade = 0;
                                estado = GAME_OVER;
                                if (som_gameover) al_play_sample(som_gameover, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                            } else {
                                if (som_explosao) al_play_sample(som_explosao, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
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
                al_draw_text(fonte, al_map_rgb(255, 215, 0), LARGURA / 2, 160, ALLEGRO_ALIGN_CENTER, "NUMBER STRIKER");
                al_draw_filled_rectangle(300, 260, 500, 320, al_map_rgb(40, 140, 70));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 286, ALLEGRO_ALIGN_CENTER, "JOGAR");
            }
            else if (estado == DIFICULDADE) {
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 130, ALLEGRO_ALIGN_CENTER, "ESCOLHA A DIFICULDADE");

                al_draw_filled_rectangle(300, 200, 500, 250, al_map_rgb(40, 140, 70));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 221, ALLEGRO_ALIGN_CENTER, "FACIL");

                al_draw_filled_rectangle(300, 280, 500, 330, al_map_rgb(200, 140, 30));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 301, ALLEGRO_ALIGN_CENTER, "MEDIO");

                al_draw_filled_rectangle(300, 360, 500, 410, al_map_rgb(180, 40, 40));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 381, ALLEGRO_ALIGN_CENTER, "DIFICIL");
            }
            else if (estado == JOGANDO) {
                desenharCidadeBlocos();

                for (int i = 0; i < MAX_METEOROS; i++) {
                    if (meteoros[i].ativo) {
                        al_draw_filled_circle(meteoros[i].x, meteoros[i].y, 26, al_map_rgb(160, 75, 40));
                        al_draw_circle(meteoros[i].x, meteoros[i].y, 26, al_map_rgb(255, 140, 50), 2);

                        char texto_conta[32];
                        sprintf_s(texto_conta, sizeof(texto_conta), "%d %c %d", meteoros[i].num1, meteoros[i].operador, meteoros[i].num2);
                        al_draw_text(fonte, al_map_rgb(255, 255, 255), meteoros[i].x, meteoros[i].y - 4, ALLEGRO_ALIGN_CENTER, texto_conta);
                    }
                }

                for (int i = 0; i < MAX_EXPLOSOES; i++) {
                    if (explosoes[i].ativo) {
                        al_draw_filled_circle(explosoes[i].x, explosoes[i].y, explosoes[i].raio, al_map_rgb(255, 80, 20));
                        al_draw_filled_circle(explosoes[i].x, explosoes[i].y, explosoes[i].raio * 0.6f, al_map_rgb(255, 210, 50));

                        char texto_res[32];
                        sprintf_s(texto_res, sizeof(texto_res), "Resp: %d", explosoes[i].resultado_perdido);
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
                al_draw_text(fonte, al_map_rgb(255, 70, 70), LARGURA / 2, 30, ALLEGRO_ALIGN_CENTER, "FIM DE JOGO - ESTATISTICAS DA PARTIDA");

                al_draw_textf(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 65, ALLEGRO_ALIGN_CENTER,
                    "Total Meteoros: %d  |  Acertos: %d  |  Erros: %d  |  Pontuacao Total: %d",
                    total_historico, acertos, erros, pontos_totais);

                al_draw_line(60, 95, LARGURA - 60, 95, al_map_rgb(120, 120, 150), 1);
                al_draw_text(fonte, al_map_rgb(200, 200, 255), 80, 105, 0, "CONTA");
                al_draw_text(fonte, al_map_rgb(200, 200, 255), 260, 105, 0, "RESULTADO");
                al_draw_text(fonte, al_map_rgb(200, 200, 255), 440, 105, 0, "STATUS");
                al_draw_text(fonte, al_map_rgb(200, 200, 255), 620, 105, 0, "PONTOS");
                al_draw_line(60, 125, LARGURA - 60, 125, al_map_rgb(120, 120, 150), 1);

                int max_visiveis = 12;
                for (int i = 0; i < max_visiveis && (i + scroll_offset) < total_historico; i++) {
                    int idx = i + scroll_offset;
                    int y_linha = 140 + (i * 28);

                    ALLEGRO_COLOR cor = historico[idx].acertou ? al_map_rgb(80, 255, 120) : al_map_rgb(255, 90, 90);

                    al_draw_text(fonte, al_map_rgb(255, 255, 255), 80, y_linha, 0, historico[idx].conta);
                    al_draw_textf(fonte, al_map_rgb(255, 255, 255), 260, y_linha, 0, "%d", historico[idx].resultado);
                    al_draw_text(fonte, cor, 440, y_linha, 0, historico[idx].acertou ? "ACERTOU" : "CAIU");
                    al_draw_textf(fonte, cor, 620, y_linha, 0, "+%d pts", historico[idx].pontos_ganhos);
                }

                if (total_historico > max_visiveis) {
                    al_draw_text(fonte, al_map_rgb(180, 180, 180), LARGURA / 2, 485, ALLEGRO_ALIGN_CENTER, "Use SETA CIMA / SETA BAIXO para rolar a lista");
                }

                al_draw_filled_rectangle(300, 520, 500, 570, al_map_rgb(40, 120, 180));
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, 541, ALLEGRO_ALIGN_CENTER, "VOLTAR AO MENU");
            }

            al_flip_display();
        }
    }

    if (som_tecla) al_destroy_sample(som_tecla);
    if (som_acerto) al_destroy_sample(som_acerto);
    if (som_explosao) al_destroy_sample(som_explosao);
    if (som_gameover) al_destroy_sample(som_gameover);

    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(display);

    return 0;
}
