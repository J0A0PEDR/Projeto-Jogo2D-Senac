#include "interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#define LARGURA 800
#define ALTURA 600
#define MAX_METEOROS 5
#define MAX_FUNDO 15
#define MAX_PREDIOS 12

typedef enum { MENU, JOGANDO, GAME_OVER } EstadoJogo;
typedef enum { FACIL, MEDIO, DIFICIL } Dificuldade;
typedef struct {
    float x, y, velocidade;
    int num1, num2, resultado, ativo;
    char operacao;
} Meteoro;
typedef struct { float x, y, velocidade, tamanho; } MeteoroFundo;
typedef struct {
    float x, y, largura, altura;
    int r, g, b;
} Predio;

static void criar_meteoro(Meteoro *m, Dificuldade d) {
    int max_num = d == FACIL ? 10 : (d == MEDIO ? 20 : 50);
    float base = d == FACIL ? 0.5f : (d == MEDIO ? 0.9f : 1.4f);
    m->x = (float)(60 + rand() % (LARGURA - 120));
    m->y = (float)(-50 - rand() % 80);
    m->velocidade = base + (float)rand() / RAND_MAX * 0.8f;
    m->num1 = 1 + rand() % max_num;
    m->num2 = 1 + rand() % max_num;
    m->operacao = rand() % 2 ? '+' : '-';
    if (m->operacao == '-' && m->num1 < m->num2) {
        int n = m->num1;
        m->num1 = m->num2;
        m->num2 = n;
    }
    m->resultado = m->operacao == '+' ? m->num1 + m->num2 : m->num1 - m->num2;
    m->ativo = 1;
}
static void criar_fundo(MeteoroFundo *m) {
    m->x = (float)(rand() % LARGURA);
    m->y = (float)(-rand() % ALTURA);
    m->velocidade = 1.5f + (float)rand() / RAND_MAX * 2.5f;
    m->tamanho = 2.0f + (float)rand() / RAND_MAX * 3.0f;
}
static void iniciar_partida(Meteoro m[], Dificuldade d, int *pontos,
                            int *vidas, char entrada[], int *indice) {
    int i;
    *pontos = 0;
    *vidas = 3;
    *indice = 0;
    entrada[0] = '\0';
    for (i = 0; i < MAX_METEOROS; ++i) {
        criar_meteoro(&m[i], d);
        m[i].y -= i * 100.0f;
    }
}
int executar_jogo(void) {
    ALLEGRO_DISPLAY *tela = NULL;
    ALLEGRO_EVENT_QUEUE *fila = NULL;
    ALLEGRO_TIMER *timer = NULL;
    ALLEGRO_FONT *fonte = NULL;
    EstadoJogo estado = MENU;
    Dificuldade dificuldade = FACIL;
    Predio predios[MAX_PREDIOS];
    MeteoroFundo fundo[MAX_FUNDO];
    Meteoro meteoros[MAX_METEOROS] = {0};
    char entrada[10] = "";
    int indice = 0, pontos = 0, vidas = 3, rodando = 1, i;
    const float painel_x = 230.0f, painel_y = 135.0f;
    const float botao_x = 300.0f, botao_y = 245.0f;

    srand((unsigned int)time(NULL));
    if (!al_init()) { fprintf(stderr, "Falha ao iniciar Allegro.\n"); return 1; }
    if (!al_init_primitives_addon()) { fprintf(stderr, "Falha em primitives.\n"); return 1; }
    al_init_font_addon();
    if (!al_install_keyboard() || !al_install_mouse()) {
        fprintf(stderr, "Falha no teclado ou mouse.\n");
        return 1;
    }
    tela = al_create_display(LARGURA, ALTURA);
    fila = al_create_event_queue();
    timer = al_create_timer(1.0 / 60.0);
    fonte = al_create_builtin_font();
    if (!tela || !fila || !timer || !fonte) {
        fprintf(stderr, "Falha ao criar recursos do Allegro.\n");
        if (fonte) al_destroy_font(fonte);
        if (timer) al_destroy_timer(timer);
        if (fila) al_destroy_event_queue(fila);
        if (tela) al_destroy_display(tela);
        return 1;
    }
    al_set_window_title(tela, "Number Striker - Defesa Matematica");
    al_register_event_source(fila, al_get_display_event_source(tela));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_mouse_event_source());

    for (i = 0; i < MAX_PREDIOS; ++i) {
        predios[i].largura = (float)(50 + rand() % 40);
        predios[i].altura = (float)(80 + rand() % 120);
        predios[i].x = (float)(10 + i * 65);
        predios[i].y = ALTURA - predios[i].altura;
        predios[i].r = 20 + rand() % 30;
        predios[i].g = 25 + rand() % 35;
        predios[i].b = 45 + rand() % 40;
    }
    for (i = 0; i < MAX_FUNDO; ++i) criar_fundo(&fundo[i]);
    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila, &ev);
        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE ||
            (ev.type == ALLEGRO_EVENT_KEY_DOWN &&
             ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
            rodando = 0;
        } else if (ev.type == ALLEGRO_EVENT_TIMER) {
            for (i = 0; i < MAX_FUNDO; ++i) {
                fundo[i].y += fundo[i].velocidade;
                if (fundo[i].y > ALTURA) {
                    criar_fundo(&fundo[i]);
                    fundo[i].y = -10;
                }
            }
            if (estado == JOGANDO) {
                for (i = 0; i < MAX_METEOROS; ++i) {
                    if (!meteoros[i].ativo) continue;
                    meteoros[i].y += meteoros[i].velocidade;
                    if (meteoros[i].y > ALTURA - 40) {
                        --vidas;
                        if (vidas <= 0) {
                            vidas = 0;
                            estado = GAME_OVER;
                            break;
                        }
                        criar_meteoro(&meteoros[i], dificuldade);
                    }
                }
            }
        } else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
                   (estado == MENU || estado == GAME_OVER)) {
            int x = ev.mouse.x, y = ev.mouse.y;
            if (x >= botao_x && x <= botao_x + 200 &&
                y >= botao_y && y <= botao_y + 45) {
                iniciar_partida(meteoros, dificuldade, &pontos, &vidas, entrada, &indice);
                estado = JOGANDO;
            } else if (estado == MENU && y >= painel_y + 60 && y <= painel_y + 85) {
                if (x >= painel_x + 20 && x <= painel_x + 100) dificuldade = FACIL;
                else if (x >= painel_x + 120 && x <= painel_x + 210) dificuldade = MEDIO;
                else if (x >= painel_x + 230 && x <= painel_x + 320) dificuldade = DIFICIL;
            }
        } else if (ev.type == ALLEGRO_EVENT_KEY_CHAR && estado == JOGANDO) {
            /* ALLEGRO_EVENT guarda o caractere em keyboard.unichar. */
            if (ev.keyboard.unichar >= '0' && ev.keyboard.unichar <= '9') {
                if (indice < (int)sizeof(entrada) - 1) {
                    entrada[indice++] = (char)ev.keyboard.unichar;
                    entrada[indice] = '\0';
                }
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && indice > 0) {
                entrada[--indice] = '\0';
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER ||
                       ev.keyboard.keycode == ALLEGRO_KEY_PAD_ENTER) {
                if (indice > 0) {
                    int resposta = atoi(entrada);
                    for (i = 0; i < MAX_METEOROS; ++i) {
                        if (meteoros[i].ativo && meteoros[i].resultado == resposta) {
                            pontos += (dificuldade + 1) * 10;
                            criar_meteoro(&meteoros[i], dificuldade);
                            break;
                        }
                    }
                    indice = 0;
                    entrada[0] = '\0';
                }
            }
        }
        if (ev.type == ALLEGRO_EVENT_TIMER && al_is_event_queue_empty(fila)) {
            al_clear_to_color(al_map_rgb(10, 10, 22));
            for (i = 0; i < MAX_PREDIOS; ++i) {
                float x, y;
                Predio *p = &predios[i];
                al_draw_filled_rectangle(p->x, p->y, p->x + p->largura, ALTURA,
                                         al_map_rgb(p->r, p->g, p->b));
                for (y = p->y + 10; y < ALTURA - 20; y += 25) {
                    for (x = p->x + 8; x < p->x + p->largura - 12; x += 15) {
                        al_draw_filled_rectangle(x, y, x + 6, y + 10,
                                                 al_map_rgb(200, 180, 80));
                    }
                }
            }
            for (i = 0; i < MAX_FUNDO; ++i) {
                al_draw_line(fundo[i].x, fundo[i].y - 20, fundo[i].x, fundo[i].y,
                             al_map_rgb(255, 100, 30), 2);
                al_draw_filled_circle(fundo[i].x, fundo[i].y, fundo[i].tamanho,
                                      al_map_rgb(255, 200, 50));
            }
            if (estado == MENU || estado == GAME_OVER) {
                const char *rotulo = estado == MENU ? "JOGAR" : "JOGAR NOVAMENTE";
                al_draw_filled_rectangle(painel_x, painel_y, painel_x + 340,
                                         painel_y + 230, al_map_rgb(20, 24, 40));
                al_draw_rectangle(painel_x, painel_y, painel_x + 340,
                                  painel_y + 230, al_map_rgb(0, 200, 255), 2);
                if (estado == MENU) {
                    al_draw_text(fonte, al_map_rgb(0, 255, 200), LARGURA / 2,
                                 painel_y + 20, ALLEGRO_ALIGN_CENTER, "NUMBER STRIKER");
                    al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA / 2,
                                 painel_y + 50, ALLEGRO_ALIGN_CENTER, "Dificuldade:");
                    al_draw_text(fonte, dificuldade == FACIL ? al_map_rgb(0, 255, 0) : al_map_rgb(100, 100, 100),
                                 painel_x + 50, painel_y + 70, ALLEGRO_ALIGN_CENTER, "[ FACIL ]");
                    al_draw_text(fonte, dificuldade == MEDIO ? al_map_rgb(255, 255, 0) : al_map_rgb(100, 100, 100),
                                 painel_x + 165, painel_y + 70, ALLEGRO_ALIGN_CENTER, "[ MEDIO ]");
                    al_draw_text(fonte, dificuldade == DIFICIL ? al_map_rgb(255, 50, 50) : al_map_rgb(100, 100, 100),
                                 painel_x + 280, painel_y + 70, ALLEGRO_ALIGN_CENTER, "[ DIFICIL ]");
                } else {
                    char final_pt[64];
                    snprintf(final_pt, sizeof final_pt, "Pontos Conquistados: %d", pontos);
                    al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA / 2,
                                 painel_y + 15, ALLEGRO_ALIGN_CENTER, "GAME OVER!");
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2,
                                 painel_y + 35, ALLEGRO_ALIGN_CENTER, final_pt);
                }
                al_draw_filled_rectangle(botao_x, botao_y, botao_x + 200,
                                         botao_y + 45, al_map_rgb(0, 180, 90));
                al_draw_rectangle(botao_x, botao_y, botao_x + 200,
                                  botao_y + 45, al_map_rgb(255, 255, 255), 2);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2,
                             botao_y + 16, ALLEGRO_ALIGN_CENTER, rotulo);
            } else if (estado == JOGANDO) {
                char texto[64];
                for (i = 0; i < MAX_METEOROS; ++i) {
                    if (!meteoros[i].ativo) continue;
                    al_draw_filled_circle(meteoros[i].x, meteoros[i].y, 32,
                                          al_map_rgb(200, 70, 20));
                    al_draw_circle(meteoros[i].x, meteoros[i].y, 32,
                                   al_map_rgb(255, 150, 0), 3);
                    snprintf(texto, sizeof texto, "%d %c %d",
                             meteoros[i].num1, meteoros[i].operacao, meteoros[i].num2);
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), meteoros[i].x,
                                 meteoros[i].y - 5, ALLEGRO_ALIGN_CENTER, texto);
                }
                al_draw_filled_rectangle(0, 0, LARGURA, 35, al_map_rgb(15, 15, 25));
                snprintf(texto, sizeof texto, "Pontos: %d   |   Vidas: %d", pontos, vidas);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), 20, 10, 0, texto);
                al_draw_filled_rectangle(0, ALTURA - 35, LARGURA, ALTURA,
                                         al_map_rgb(20, 20, 30));
                snprintf(texto, sizeof texto, "Sua resposta: %s_", entrada);
                al_draw_text(fonte, al_map_rgb(0, 255, 150),
                             LARGURA - 220, ALTURA - 22, 0, texto);
            }
            al_flip_display();
        }
    }
    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    al_destroy_display(tela);
    al_shutdown_primitives_addon();
    return 0;
}
