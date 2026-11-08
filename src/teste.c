#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#define LARGURA 800
#define ALTURA 600
#define MAX_METEOROS_JOGO 5
#define MAX_METEOROS_FUNDO 15
#define MAX_PREDIOS 14

typedef enum { MENU, CONTAGEM, JOGANDO, GAME_OVER } EstadoJogo;
typedef enum { FACIL, MEDIO, DIFICIL } Dificuldade;

typedef struct {
    float x, y;
    float velocidade;
    int num1, num2;
    char operacao;
    int resultado;
    bool ativo;
    float raio;
} Meteoro;

typedef struct {
    float x, y;
    float velocidade;
    float tamanho;
} MeteoroFundo;

typedef struct {
    float x, y, largura, altura;
    int r, g, b;
} Predio;

typedef struct {
    float x1, y1, x2, y2;
    int tempo_vida;
} FeixeLaser;

// Protótipos
void criar_meteoro_jogo(Meteoro *m, Dificuldade diff);
void criar_meteoro_fundo(MeteoroFundo *m);

void criar_meteoro_jogo(Meteoro *m, Dificuldade diff) {
    m->x = 60 + rand() % (LARGURA - 120);
    m->y = -50 - (rand() % 100);
    m->raio = 30.0f;

    float vel_base = 0.6f;
    int max_val = 10;

    m->velocidade = vel_base + ((float)rand() / RAND_MAX) * 0.7f;
    m->num1 = 1 + rand() % max_val;
    m->num2 = 1 + rand() % max_val;

    if (rand() % 2 == 0) {
        m->operacao = '+';
        m->resultado = m->num1 + m->num2;
    } else {
        m->operacao = '-';
        if (m->num1 < m->num2) {
            int temp = m->num1;
            m->num1 = m->num2;
            m->num2 = temp;
        }
        m->resultado = m->num1 - m->num2;
    }
    m->ativo = true;
}

void criar_meteoro_fundo(MeteoroFundo *m) {
    m->x = rand() % LARGURA;
    m->y = -(rand() % ALTURA);
    m->velocidade = 1.5f + ((float)rand() / RAND_MAX) * 2.5f;
    m->tamanho = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
}

int main() {
    srand((unsigned int)time(NULL));

    if (!al_init()) return -1;
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_keyboard();
    al_install_mouse();

    ALLEGRO_DISPLAY *display = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    ALLEGRO_TIMER *timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_FONT *fonte = al_create_builtin_font();

    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    EstadoJogo estado = MENU;
    Dificuldade diff_atual = FACIL;

    // Variáveis para a contagem regressiva nos modos bloqueados
    int contagem_val = 3;
    int contagem_frames = 0;

    // Inicializar Cidade no Fundo
    Predio predios[MAX_PREDIOS];
    for (int i = 0; i < MAX_PREDIOS; i++) {
        predios[i].largura = 45 + rand() % 35;
        predios[i].altura = 70 + rand() % 110;
        predios[i].x = (i * 58) + 5;
        predios[i].y = ALTURA - predios[i].altura;
        predios[i].r = 20 + rand() % 25;
        predios[i].g = 25 + rand() % 30;
        predios[i].b = 40 + rand() % 35;
    }

    // Inicializar Meteoros do Fundo
    MeteoroFundo meteoros_fundo[MAX_METEOROS_FUNDO];
    for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
        criar_meteoro_fundo(&meteoros_fundo[i]);
    }

    Meteoro meteoros[MAX_METEOROS_JOGO];
    FeixeLaser laser = { 0, 0, 0, 0, 0 };

    char entrada_texto[10] = "";
    int indice_texto = 0;
    int pontos = 0;
    int vidas = 3;

    // Dimensões da Interface do Menu
    float panel_w = 360, panel_h = 240;
    float panel_x = (LARGURA - panel_w) / 2.0f;
    float panel_y = (ALTURA - panel_h) / 2.0f - 50;

    float btn_play_w = 220, btn_play_h = 45;
    float btn_play_x = (LARGURA - btn_play_w) / 2.0f;
    float btn_play_y = panel_y + 125;

    bool rodando = true;
    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            // Animação contínua do fundo
            for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
                meteoros_fundo[i].y += meteoros_fundo[i].velocidade;
                if (meteoros_fundo[i].y > ALTURA) {
                    criar_meteoro_fundo(&meteoros_fundo[i]);
                    meteoros_fundo[i].y = -10;
                }
            }

            // Lógica da Contagem Regressiva (Para Médio/Difícil)
            if (estado == CONTAGEM) {
                contagem_frames++;
                if (contagem_frames >= 60) { // 60 frames = 1 segundo
                    contagem_frames = 0;
                    contagem_val--;
                    if (contagem_val <= 0) {
                        estado = MENU; // Retorna para a interface principal
                    }
                }
            }

            // Laser
            if (laser.tempo_vida > 0) {
                laser.tempo_vida--;
            }

            // Física do Jogo (Somente no estado JOGANDO)
            if (estado == JOGANDO) {
                if (vidas > 0) {
                    for (int i = 0; i < MAX_METEOROS_JOGO; i++) {
                        if (meteoros[i].ativo) {
                            meteoros[i].y += meteoros[i].velocidade;
                            if (meteoros[i].y > ALTURA - 40) {
                                vidas--;
                                criar_meteoro_jogo(&meteoros[i], diff_atual);
                                if (vidas <= 0) estado = GAME_OVER;
                            }
                        }
                    }
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = ev.mouse.x;
            int my = ev.mouse.y;

            if (estado == MENU || estado == GAME_OVER) {
                // Clique no Botão JOGAR
                if (mx >= btn_play_x && mx <= btn_play_x + btn_play_w &&
                    my >= btn_play_y && my <= btn_play_y + btn_play_h) {
                    
                    if (diff_atual == FACIL) {
                        // Inicia o jogo normalmente apenas no modo FÁCIL
                        pontos = 0;
                        vidas = 3;
                        indice_texto = 0;
                        entrada_texto[0] = '\0';
                        laser.tempo_vida = 0;

                        for (int i = 0; i < MAX_METEOROS_JOGO; i++) {
                            criar_meteoro_jogo(&meteoros[i], diff_atual);
                        }
                        estado = JOGANDO;
                    } else {
                        // Ativa a contagem de aviso para os modos MÉDIO e DIFÍCIL
                        contagem_val = 3;
                        contagem_frames = 0;
                        estado = CONTAGEM;
                    }
                }

                // Seleção de Dificuldade
                if (estado == MENU && my >= panel_y + 70 && my <= panel_y + 95) {
                    if (mx >= panel_x + 20 && mx <= panel_x + 110) diff_atual = FACIL;
                    if (mx >= panel_x + 130 && mx <= panel_x + 230) diff_atual = MEDIO;
                    if (mx >= panel_x + 240 && mx <= panel_x + 340) diff_atual = DIFICIL;
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
            if (estado == JOGANDO) {
                if (ev.unichar >= '0' && ev.unichar <= '9') {
                    if (indice_texto < (int)sizeof(entrada_texto) - 1) {
                        entrada_texto[indice_texto++] = (char)ev.unichar;
                        entrada_texto[indice_texto] = '\0';
                    }
                }
                else if (ev.keycode == ALLEGRO_KEY_BACKSPACE && indice_texto > 0) {
                    entrada_texto[--indice_texto] = '\0';
                }
                else if (ev.keycode == ALLEGRO_KEY_ENTER || ev.keycode == ALLEGRO_KEY_PAD_ENTER) {
                    if (indice_texto > 0) {
                        int resposta = atoi(entrada_texto);
                        for (int i = 0; i < MAX_METEOROS_JOGO; i++) {
                            if (meteoros[i].ativo && meteoros[i].resultado == resposta) {
                                pontos += 10;
                                
                                laser.x1 = LARGURA / 2.0f;
                                laser.y1 = ALTURA - 30.0f;
                                laser.x2 = meteoros[i].x;
                                laser.y2 = meteoros[i].y;
                                laser.tempo_vida = 10;

                                criar_meteoro_jogo(&meteoros[i], diff_atual);
                                break;
                            }
                        }
                        indice_texto = 0;
                        entrada_texto[0] = '\0';
                    }
                }
            }
        }

        // RENDERIZAÇÃO
        if (ev.type == ALLEGRO_EVENT_TIMER && al_is_event_queue_empty(queue)) {
            al_clear_to_color(al_map_rgb(10, 12, 24));

            // 1. Cidade
            for (int i = 0; i < MAX_PREDIOS; i++) {
                al_draw_filled_rectangle(predios[i].x, predios[i].y, 
                                        predios[i].x + predios[i].largura, ALTURA, 
                                        al_map_rgb(predios[i].r, predios[i].g, predios[i].b));
                for (float wy = predios[i].y + 12; wy < ALTURA - 25; wy += 22) {
                    for (float wx = predios[i].x + 6; wx < predios[i].x + predios[i].largura - 10; wx += 14) {
                        al_draw_filled_rectangle(wx, wy, wx + 5, wy + 8, al_map_rgb(220, 190, 80));
                    }
                }
            }

            // 2. Meteoros do fundo
            for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
                al_draw_line(meteoros_fundo[i].x, meteoros_fundo[i].y - 18, 
                             meteoros_fundo[i].x, meteoros_fundo[i].y, 
                             al_map_rgb(255, 90, 20), 2);
                al_draw_filled_circle(meteoros_fundo[i].x, meteoros_fundo[i].y, 
                                       meteoros_fundo[i].tamanho, al_map_rgb(255, 210, 60));
            }

            // 3. MENU / GAME OVER
            if (estado == MENU || estado == GAME_OVER) {
                al_draw_filled_rectangle(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, al_map_rgb(18, 22, 38));
                al_draw_rectangle(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, al_map_rgb(0, 220, 255), 2);

                if (estado == MENU) {
                    al_draw_text(fonte, al_map_rgb(0, 255, 200), LARGURA / 2, panel_y + 20, ALLEGRO_ALIGN_CENTER, "NUMBER STRIKER");
                    al_draw_text(fonte, al_map_rgb(180, 180, 200), LARGURA / 2, panel_y + 38, ALLEGRO_ALIGN_CENTER, "DEFESA MATEMATICA");

                    al_draw_text(fonte, al_map_rgb(220, 220, 220), LARGURA / 2, panel_y + 60, ALLEGRO_ALIGN_CENTER, "Dificuldade:");
                    al_draw_text(fonte, (diff_atual == FACIL) ? al_map_rgb(0, 255, 100) : al_map_rgb(100, 100, 120), 
                                 panel_x + 60, panel_y + 80, ALLEGRO_ALIGN_CENTER, "[ FACIL ]");
                    al_draw_text(fonte, (diff_atual == MEDIO) ? al_map_rgb(255, 255, 0) : al_map_rgb(100, 100, 120), 
                                 panel_x + 180, panel_y + 80, ALLEGRO_ALIGN_CENTER, "[ MEDIO ]");
                    al_draw_text(fonte, (diff_atual == DIFICIL) ? al_map_rgb(255, 60, 60) : al_map_rgb(100, 100, 120), 
                                 panel_x + 290, panel_y + 80, ALLEGRO_ALIGN_CENTER, "[ DIFICIL ]");
                } else {
                    al_draw_text(fonte, al_map_rgb(255, 50, 50), LARGURA / 2, panel_y + 25, ALLEGRO_ALIGN_CENTER, "GAME OVER!");
                    char final_pt[50];
                    sprintf(final_pt, "Pontuacao Final: %d", pontos);
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, panel_y + 55, ALLEGRO_ALIGN_CENTER, final_pt);
                }

                al_draw_filled_rectangle(btn_play_x, btn_play_y, btn_play_x + btn_play_w, btn_play_y + btn_play_h, al_map_rgb(0, 180, 90));
                al_draw_rectangle(btn_play_x, btn_play_y, btn_play_x + btn_play_w, btn_play_y + btn_play_h, al_map_rgb(255, 255, 255), 2);
                
                const char* btn_label = (estado == MENU) ? "JOGAR" : "JOGAR NOVAMENTE";
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, btn_play_y + 17, ALLEGRO_ALIGN_CENTER, btn_label);
            }

            // 4. CONTAGEM REGRESSIVA (Modos Médio e Difícil)
            else if (estado == CONTAGEM) {
                al_draw_filled_rectangle(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, al_map_rgb(18, 22, 38));
                al_draw_rectangle(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, al_map_rgb(255, 180, 0), 2);

                al_draw_text(fonte, al_map_rgb(255, 200, 0), LARGURA / 2, panel_y + 30, ALLEGRO_ALIGN_CENTER, "EM BREVE!");
                al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA / 2, panel_y + 60, ALLEGRO_ALIGN_CENTER, "Modo em desenvolvimento");
                al_draw_text(fonte, al_map_rgb(180, 180, 180), LARGURA / 2, panel_y + 85, ALLEGRO_ALIGN_CENTER, "Retornando ao Menu em:");

                char cont_str[10];
                sprintf(cont_str, "%d", contagem_val);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, panel_y + 140, ALLEGRO_ALIGN_CENTER, cont_str);
            }

            // 5. JOGO ATIVO
            else if (estado == JOGANDO) {
                al_draw_filled_triangle(LARGURA / 2 - 20, ALTURA - 30, 
                                        LARGURA / 2 + 20, ALTURA - 30, 
                                        LARGURA / 2, ALTURA - 55, al_map_rgb(0, 200, 255));

                if (laser.tempo_vida > 0) {
                    al_draw_line(laser.x1, laser.y1, laser.x2, laser.y2, al_map_rgb(0, 255, 255), 4);
                    al_draw_line(laser.x1, laser.y1, laser.x2, laser.y2, al_map_rgb(255, 255, 255), 1);
                }

                for (int i = 0; i < MAX_METEOROS_JOGO; i++) {
                    if (meteoros[i].ativo) {
                        al_draw_filled_circle(meteoros[i].x, meteoros[i].y, meteoros[i].raio, al_map_rgb(190, 60, 20));
                        al_draw_circle(meteoros[i].x, meteoros[i].y, meteoros[i].raio, al_map_rgb(255, 140, 0), 3);

                        char conta_str[20];
                        sprintf(conta_str, "%d %c %d", meteoros[i].num1, meteoros[i].operacao, meteoros[i].num2);
                        al_draw_text(fonte, al_map_rgb(255, 255, 255), meteoros[i].x, meteoros[i].y - 5, ALLEGRO_ALIGN_CENTER, conta_str);
                    }
                }

                al_draw_filled_rectangle(0, 0, LARGURA, 35, al_map_rgb(12, 14, 24));
                char hud_str[60];
                sprintf(hud_str, "Pontos: %d   |   Vidas: %d", pontos, vidas);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), 20, 10, 0, hud_str);

                al_draw_filled_rectangle(0, ALTURA - 35, LARGURA, ALTURA, al_map_rgb(18, 20, 32));
                char input_str[30];
                sprintf(input_str, "Sua resposta: %s_", entrada_texto);
                al_draw_text(fonte, al_map_rgb(0, 255, 150), LARGURA - 220, ALTURA - 22, 0, input_str);
            }

            al_flip_display();
        }
    }

    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}
