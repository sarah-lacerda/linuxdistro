# Trabalho Prático 2



Este tutorial contém o processo básico usado para desenvolver o trabalho prático 2 da disciplina de Laboratório de Sistemas Operacionais (CC). Este trabalho consiste na implementação de um escalonador de disco, baseado no algoritmo de SSTF (Shortest Seek Time First). O escalonador deve ser implementado como um módulo compatível com o Linux kernel 4.13.9. Após a implementação desse algoritmo, o desempenho do mesmo deve ser comparado com o desempenho de um algoritmo de First Come, First Served.



Para este trabalho, a implementação do escalonador será em C.



Para mais informações, consulte o enunciado original presente neste anexo (nomeado como 'tp2.pdf').





# Implementação do escalonador



A implementação do escalonador foi com ponto de partida no esqueleto de código fornecido aula, disponível em https://github.com/miguelxvr/sstf-iosched-skeleton.git. A partir daí foram adicionados os seguintes métodos:



## sstf_get_next_rq



É a função principal do algoritmo, responsável por determinar qual é o próximo request de acesso ao disco que será processado. A nossa implememtação considerou que o disco é um vetor simples, com início e final que não estão conectados entre si. Assim, caso o cabeçote do disco esteja na última posição e precise acessar a primeira posição, ele terá de cruzar todo o disco. Uma implementação diferente é sugerida mais abaixo.



O código da função é o seguinte:



```

static void sstf_get_next_rq(struct sstf_data *nd, struct request **next){

        unsigned long long int min_blk_distance = ULLONG_MAX;

        struct request *currentRq = NULL;

        struct list_head* position; struct list_head* temp; list_for_each_safe(position, temp, &nd->queue) {

                struct request *rq = list_entry(position, struct request, queuelist);

                unsigned long long int blk_distance = abs(blk_rq_pos(rq) - last_blk_pos_dispatched);



                if (blk_distance < min_blk_distance) {

                        min_blk_distance = blk_distance;

                        currentRq = rq;

                }



        }



        *next = currentRq;

}



```

A variável min_blk_distance é inicializada com o valor máximo de um long long e é usada para encontrar a distãncia mínima que existirá entre a atual posição do cabeçote e as requisições que estão esperando na fila. O método list_for_each_safe é um método disponível na implementação de listas ligadas do kernel Linux. Ele itera pela lista de requests e, para cada uma delas, calcula a distância entre a posição atual do cabeçote e a posição a ser acessada na requisição, com a seguinte fórmula:



```abs(blk_rq_pos(rq) - last_blk_pos_dispatched)

```

O uso do valor absoluto garante que a distância estará em uma mesma escala, seja para uma posição de memória à esquerda ou à direita da posição atual.



O algoritmo então encontra a menor distância entre a posição atual e um dos requests e salva essa requisição na variável next, que, em seguida, é utilizada no método sstf_dispatch, descrito abaixo.



## sstf_dispatch



É a função que faz o dispatch da requisição para o disco. A variável rq é inicializada com o valor nulo e chama a função sstf_get_next_rq descrita acima para que essa encontre qual é o próximo request a ser servido, com base no algoritmo de escalonamento. Note-se o uso da função list_del_init para a retirada do request que está sendo servido da lista de requests. Esse método também está é um método disponível na implementação de listas ligadas do kernel Linux



```

static int sstf_dispatch(struct request_queue *q, int force){

        struct sstf_data *nd = q->elevator->elevator_data;

        struct request *rq = NULL;

        sstf_get_next_rq(nd, &rq);





        /* Aqui deve-se retirar uma requisição da fila e enviá-la para processamento.

         * Use como exemplo o driver noop-iosched.c. Veja como a requisição é tratada.

         *

         * Antes de retornar da função, imprima o sector que foi atendido.

         */

        if (rq) {

                list_del_init(&rq->queuelist);

                elv_dispatch_sort(q, rq);



                char direction = sstf_get_cursor_direction(last_blk_pos_dispatched, blk_rq_pos(rq));

                last_blk_pos_dispatched = blk_rq_pos(rq);

                printk(KERN_EMERG "[SSTF] dsp %c %llu\n", direction, blk_rq_pos(rq));



                return 1;

        }

        return 0;

}



```



Depois que o request é servido, a posição do cabeçote é atualizada, ficando salva na variável last_blk_pos_dispatched, que é usada no método sstf_get_next_rq. Note-se que, para a primeira execução, a variável last_blk_pos_dispatched é inicializada com o valor 0.



## Outra implementação para o algoritmo do escalonador



Acaso desejado adaptar o algoritmo para considerar o disco como um vetor com ligação do início e do fim, seria necessário calcular, para cada elemento da lista de requests, a distância do cabeçote e do elemento considerando a seguinte equação:



MIN(ABS(posição_do_cabeçote - posição_do_request), (tamanho_máximo_do_disco + posição_do_request - posição_do_cabeçote)).





# Tutorial de execução



A implementação do módulo seguiu as instruções dos tutoriais disponibilizados na disciplina, em especial os tutoriais X e Y.





# Avaliação de desempenho



Com o output da execução do sector_read, é possível fazermos a análise de desempenho do algoritmo implementado, quando comparado com uma implementação First Come First Served. Para isso, foram feitos gráficos que representam a ordem da adição das requisições à fila (que representam como seria o tratamento das requisições por um algoritmo FCFS) e a ordem da execução das requisições pelo algoritmo implementado. Os gráficos se encontram nas figuras X e Y, juntadas ao repositório. A análise visual dos gráficos já traz a intuição de que as posições das requisições servidas pelo algoritmo implementado são muito mais próximas uma da outra do que seriam no caso do algoritmo FCFS.



Para formalizar a intuição em um parâmetro objetivo, foi calculado a distância total percorrida pelo cabeçote para cada um dos algoritmos. A distãncia total para o FCFS foi de X, enquanto para o algoritmo implementado foi de Y. Ou seja, o FCFS exigiu Z vezes mais trabalho do que o algoritmo implementado.





## Autora



**Sarah Lacerda / 2022.2**

Código disponibilizado em: https://github.com/sarah-lacerda/linuxdistro/tree/tp1
