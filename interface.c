#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#define LARGURA 800
#define ALTURA 600
#define MAX_METEOROS_JOGO 5
#define MAX_METEOROS_FUNDO 15
#define MAX_PREDIOS 12

// Estados do Jogo
typedef enum { MENU, JOGANDO, GAME_OVER } EstadoJogo;
typedef enum { FACIL, MEDIO, DIFICIL } Dificuldade;

// Estrutura do Meteoro Principal do Jogo
typedef struct {
    float x, y;
    float velocidade;
    int num1, num2;
    char operacao;
    int resultado;
    bool ativo;
} Meteoro;

// Estrutura dos Meteoros Decorativos do Fundo
typedef struct {
    float x, y;
    float velocidade;
    float tamanho;
} MeteoroFundo;

// Estrutura dos Prédios (Cidade no Fundo)
typedef struct {
    float x, y, largura, altura;
    int cor_r, cor_g, cor_b;
} Predio;

// Função para gerar meteoro do jogo baseado na dificuldade
void criar_meteoro_jogo(Meteoro *m, Dificuldade diff) {
    m->x = 60 + rand() % (LARGURA - 120);
    m->y = -50 - (rand() % 80);
    
    float vel_base = 0.5f;
    int max_num = 10;

    if (diff == MEDIO) {
        vel_base = 0.9f;
        max_num = 20;
    } else if (diff == DIFICIL) {
        vel_base = 1.4f;
        max_num = 50;
    }

    m->velocidade = vel_base + ((float)rand() / RAND_MAX) * 0.8f;
    m->num1 = 1 + rand() % max_num;
    m->num2 = 1 + rand() % max_num;
    
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

// Inicializa meteoros decorativos do menu
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

    // Variáveis Globais de Estado
    EstadoJogo estado = MENU;
    Dificuldade diff_atual = FACIL;

    // Criar Cidade de Fundo (Prédios)
    Predio predios[MAX_PREDIOS];
    for (int i = 0; i < MAX_PREDIOS; i++) {
        predios[i].largura = 50 + rand() % 40;
        predios[i].altura = 80 + rand() % 120;
        predios[i].x = (i * 65) + 10;
        predios[i].y = ALTURA - predios[i].altura;
        predios[i].cor_r = 20 + rand() % 30;
        predios[i].cor_g = 25 + rand() % 35;
        predios[i].cor_b = 45 + rand() % 40;
    }

    // Criar Meteoros do Fundo
    MeteoroFundo meteoros_fundo[MAX_METEOROS_FUNDO];
    for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
        criar_meteoro_fundo(&meteoros_fundo[i]);
    }

    // Meteoros e Dados do Jogo
    Meteoro meteoros[MAX_METEOROS_JOGO];
    char entrada_texto[10] = "";
    int indice_texto = 0;
    int pontos = 0;
    int vidas = 3;

    // Posições dos Botões da Interface (Menu Centralizado Ligeiramente para Cima)
    float panel_w = 340, panel_h = 230;
    float panel_x = (LARGURA - panel_w) / 2.0f;
    float panel_y = (ALTURA - panel_h) / 2.0f - 50; // Posicionado acima do centro

    float btn_play_w = 200, btn_play_h = 45;
    float btn_play_x = (LARGURA - btn_play_w) / 2.0f;
    float btn_play_y = panel_y + 110;

    bool rodando = true;
    al_start_timer(timer);

    while (rodando) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }

        // --- ATUALIZAÇÕES POR TIMER (60 FPS) ---
        else if (ev.type == ALLEGRO_EVENT_TIMER) {
            // Animação constante do Fundo (Meteoros caindo na cidade)
            for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
                meteoros_fundo[i].y += meteoros_fundo[i].velocidade;
                if (meteoros_fundo[i].y > ALTURA) {
                    criar_meteoro_fundo(&meteoros_fundo[i]);
                    meteoros_fundo[i].y = -10;
                }
            }

            // Lógica do Jogo Rodando
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

        // --- INTERAÇÕES DE MOUSE (MENU) ---
        else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int mx = ev.mouse.x;
            int my = ev.mouse.y;

            if (estado == MENU || estado == GAME_OVER) {
                // Clique no Botão JOGAR
                if (mx >= btn_play_x && mx <= btn_play_x + btn_play_w &&
                    my >= btn_play_y && my <= btn_play_y + btn_play_h) {
                    
                    // Reseta variáveis e inicia a partida
                    pontos = 0;
                    vidas = 3;
                    indice_texto = 0;
                    entrada_texto[0] = '\0';
                    for (int i = 0; i < MAX_METEOROS_JOGO; i++) {
                        criar_meteoro_jogo(&meteoros[i], diff_atual);
                    }
                    estado = JOGANDO;
                }

                // Seleção de Dificuldade (Apenas no MENU)
                if (estado == MENU && my >= panel_y + 60 && my <= panel_y + 85) {
                    if (mx >= panel_x + 20 && mx <= panel_x + 100) diff_atual = FACIL;
                    if (mx >= panel_x + 120 && mx <= panel_x + 210) diff_atual = MEDIO;
                    if (mx >= panel_x + 230 && mx <= panel_x + 320) diff_atual = DIFICIL;
                }
            }
        }

        // --- ENTRADA DE TECLADO (DIGITAÇÃO NO JOGO) ---
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
                                pontos += (diff_atual + 1) * 10;
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

        // --- RENDERIZAÇÃO (TELA) ---
        if (ev.type == ALLEGRO_EVENT_TIMER && al_is_event_queue_empty(queue)) {
            al_clear_to_color(al_map_rgb(10, 10, 22)); // Céu noturno

            // 1. Desenhar Cidade no Fundo
            for (int i = 0; i < MAX_PREDIOS; i++) {
                al_draw_filled_rectangle(predios[i].x, predios[i].y, 
                                        predios[i].x + predios[i].largura, ALTURA, 
                                        al_map_rgb(predios[i].cor_r, predios[i].cor_g, predios[i].cor_b));
                // Janelas iluminadas
                for (float wy = predios[i].y + 10; wy < ALTURA - 20; wy += 25) {
                    for (float wx = predios[i].x + 8; wx < predios[i].x + predios[i].largura - 12; wx += 15) {
                        al_draw_filled_rectangle(wx, wy, wx + 6, wy + 10, al_map_rgb(200, 180, 80));
                    }
                }
            }

            // 2. Desenhar Meteoros Decorativos Caindo
            for (int i = 0; i < MAX_METEOROS_FUNDO; i++) {
                al_draw_line(meteoros_fundo[i].x, meteoros_fundo[i].y - 20, 
                             meteoros_fundo[i].x, meteoros_fundo[i].y, 
                             al_map_rgb(255, 100, 30), 2);
                al_draw_filled_circle(meteoros_fundo[i].x, meteoros_fundo[i].y, 
                                       meteoros_fundo[i].tamanho, al_map_rgb(255, 200, 50));
            }

            // 3. Renderização do MENU INICIAL ou GAME OVER
            if (estado == MENU || estado == GAME_OVER) {
                // Painel Central Embutido
                al_draw_filled_rectangle(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, al_map_rgb(20, 24, 40));
                al_draw_rectangle(panel_x, panel_y, panel_x + panel_w, panel_y + panel_h, al_map_rgb(0, 200, 255), 2);

                // Título
                if (estado == MENU) {
                    al_draw_text(fonte, al_map_rgb(0, 255, 200), LARGURA / 2, panel_y + 20, ALLEGRO_ALIGN_CENTER, "NUMBER STRIKER");
                } else {
                    al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA / 2, panel_y + 15, ALLEGRO_ALIGN_CENTER, "GAME OVER!");
                    char final_pt[40];
                    sprintf(final_pt, "Pontos Conquistados: %d", pontos);
                    al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, panel_y + 35, ALLEGRO_ALIGN_CENTER, final_pt);
                }

                // Opções de Dificuldade
                if (estado == MENU) {
                    al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA / 2, panel_y + 50, ALLEGRO_ALIGN_CENTER, "Dificuldade:");
                    
                    al_draw_text(fonte, (diff_atual == FACIL) ? al_map_rgb(0, 255, 0) : al_map_rgb(100, 100, 100), 
                                 panel_x + 50, panel_y + 70, ALLEGRO_ALIGN_CENTER, "[ FACIL ]");
                    al_draw_text(fonte, (diff_atual == MEDIO) ? al_map_rgb(255, 255, 0) : al_map_rgb(100, 100, 100), 
                                 panel_x + 165, panel_y + 70, ALLEGRO_ALIGN_CENTER, "[ MEDIO ]");
                    al_draw_text(fonte, (diff_atual == DIFICIL) ? al_map_rgb(255, 50, 50) : al_map_rgb(100, 100, 100), 
                                 panel_x + 280, panel_y + 70, ALLEGRO_ALIGN_CENTER, "[ DIFICIL ]");
                }

                // Botão de JOGAR / JOGAR NOVAMENTE
                al_draw_filled_rectangle(btn_play_x, btn_play_y, btn_play_x + btn_play_w, btn_play_y + btn_play_h, al_map_rgb(0, 180, 90));
                al_draw_rectangle(btn_play_x, btn_play_y, btn_play_x + btn_play_w, btn_play_y + btn_play_h, al_map_rgb(255, 255, 255), 2);
                
                const char* btn_label = (estado == MENU) ? "JOGAR" : "JOGAR NOVAMENTE";
                al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA / 2, btn_play_y + 16, ALLEGRO_ALIGN_CENTER, btn_label);
            }

            // 4. Renderização do JOGO ATIVO
            else if (estado == JOGANDO) {
                // Meteoros com equações
                for (int i = 0; i < MAX_METEOROS_JOGO; i++) {
                    if (meteoros[i].ativo) {
                        al_draw_filled_circle(meteoros[i].x, meteoros[i].y, 32, al_map_rgb(200, 70, 20));
                        al_draw_circle(meteoros[i].x, meteoros[i].y, 32, al_map_rgb(255, 150, 0), 3);

                        char conta_str[20];
                        sprintf(conta_str, "%d %c %d", meteoros[i].num1, meteoros[i].operacao, meteoros[i].num2);
                        al_draw_text(fonte, al_map_rgb(255, 255, 255), meteoros[i].x, meteoros[i].y - 5, ALLEGRO_ALIGN_CENTER, conta_str);
                    }
                }

                // Barra Superior do HUD (Pontos e Vidas)
                al_draw_filled_rectangle(0, 0, LARGURA, 35, al_map_rgb(15, 15, 25));
                char hud_str[60];
                sprintf(hud_str, "Pontos: %d   |   Vidas: %d", pontos, vidas);
                al_draw_text(fonte, al_map_rgb(255, 255, 255), 20, 10, 0, hud_str);

                // Barra Inferior de Entrada (Sua Resposta)
                al_draw_filled_rectangle(0, ALTURA - 35, LARGURA, ALTURA, al_map_rgb(20, 20, 30));
                char input_str[30];
                sprintf(input_str, "Sua resposta: %s_", entrada_texto);
                al_draw_text(fonte, al_map_rgb(0, 255, 150), LARGURA - 220, ALTURA - 22, 0, input_str);
            }

            al_flip_display();
        }
    }

    // Limpeza final
    al_destroy_font(fonte);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}
