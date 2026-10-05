#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

int main()
{
   
    if (!al_init())
    {
        printf("Erro ao iniciar o Allegro!\n");
        return -1;
    }

    if (!al_install_keyboard())
    {
        printf("Erro ao iniciar o teclado!\n");
        return -1;
    }

    al_init_font_addon();

    ALLEGRO_DISPLAY* janela = al_create_display(800, 600);

    if (janela == NULL)
    {
        printf("Erro ao criar a janela!\n");
        return -1;
    }

    ALLEGRO_FONT* fonte = al_create_builtin_font();

    if (fonte == NULL)
    {
        printf("Erro ao criar a fonte!\n");

        al_destroy_display(janela);

        return -1;
    }

    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    if (fila == NULL)
    {
        printf("Erro ao criar a fila de eventos!\n");

        al_destroy_font(fonte);
        al_destroy_display(janela);

        return -1;
    }

    al_register_event_source(
        fila,
        al_get_keyboard_event_source()
    );

    srand((unsigned int)time(NULL));

    int numeroObjetivo = rand() % 20 + 1;

    int numero1 = 0;
    int numero2 = 0;

    int etapa = 1;

    int resultado = 0;

    int executando = 1;

    while (executando)
    {
        ALLEGRO_EVENT evento;

        al_clear_to_color(al_map_rgb(30, 30, 30));

        char texto[100];

        sprintf_s(
            texto,
            sizeof(texto),
            "Numero: %d",
            numeroObjetivo
        );

        al_draw_text(
            fonte,
            al_map_rgb(255, 255, 255),
            400,
            100,
            ALLEGRO_ALIGN_CENTER,
            texto
        );

        sprintf_s(
            texto,
            sizeof(texto),
            "Primeiro numero: %d",
            numero1
        );

        al_draw_text(
            fonte,
            al_map_rgb(255, 255, 255),
            400,
            200,
            ALLEGRO_ALIGN_CENTER,
            texto
        );

        sprintf_s(
            texto,
            sizeof(texto),
            "Segundo numero: %d",
            numero2
        );

        al_draw_text(
            fonte,
            al_map_rgb(255, 255, 255),
            400,
            250,
            ALLEGRO_ALIGN_CENTER,
            texto
        );

        if (etapa == 1)
        {
            al_draw_text(
                fonte,
                al_map_rgb(255, 255, 0),
                400,
                350,
                ALLEGRO_ALIGN_CENTER,
                "Digite um numero de 0 a 9"
            );
        }
        else if (etapa == 2)
        {
            al_draw_text(
                fonte,
                al_map_rgb(255, 255, 0),
                400,
                350,
                ALLEGRO_ALIGN_CENTER,
                "Digite o segundo numero"
            );
        }
        else
        {
            if (resultado == 1)
            {
                al_draw_text(
                    fonte,
                    al_map_rgb(0, 255, 0),
                    400,
                    350,
                    ALLEGRO_ALIGN_CENTER,
                    "ACERTOU!"
                );
            }
            else
            {
                al_draw_text(
                    fonte,
                    al_map_rgb(255, 0, 0),
                    400,
                    350,
                    ALLEGRO_ALIGN_CENTER,
                    "ERROU!"
                );
            }

            al_draw_text(
                fonte,
                al_map_rgb(255, 255, 255),
                400,
                400,
                ALLEGRO_ALIGN_CENTER,
                "Pressione ENTER para jogar novamente"
            );
        }

       
        al_flip_display();
      
        al_wait_for_event(fila, &evento);

        if (evento.type == ALLEGRO_EVENT_KEY_DOWN)
        {
      
            if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            {
                executando = 0;
            }

            else if (etapa == 1)
            {
                if (evento.keyboard.keycode >= ALLEGRO_KEY_0 &&
                    evento.keyboard.keycode <= ALLEGRO_KEY_9)
                {
                    numero1 =
                        evento.keyboard.keycode - ALLEGRO_KEY_0;

                    etapa = 2;
                }
            }

            else if (etapa == 2)
            {
                if (evento.keyboard.keycode >= ALLEGRO_KEY_0 &&
                    evento.keyboard.keycode <= ALLEGRO_KEY_9)
                {
                    numero2 =
                        evento.keyboard.keycode - ALLEGRO_KEY_0;

                    // Verificar resposta
                    if (numero1 + numero2 == numeroObjetivo)
                    {
                        resultado = 1;
                    }
                    else
                    {
                        resultado = 0;
                    }

                    etapa = 3;
                }
            }

            else if (etapa == 3)
            {
                if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER)
                {
                    numeroObjetivo = rand() % 20 + 1;

                    numero1 = 0;
                    numero2 = 0;

                    etapa = 1;
                }
            }
        }
    }


    al_destroy_event_queue(fila);

    al_destroy_font(fonte);

    al_destroy_display(janela);

    return 0;
}
