# Pares 

<del>Os peers devem primeiro alocar espaço para o(s) arquivo(s). É necessário, pois, o peer não baixará as partes em ordem, então a aplicação deverá juntar esses partes na ordem correta depois de receber.</del>
Um peer pode agir como um leecher, cliente e servidor, ou como um seeder, apenas servidor.
<del>Os peers compartilham os arquivos entre si usando a técnica swarming.</del>
Peers baixam partes do conteúdo de vários peers ao mesmo tempo. Além disso, enviam ao mesmo tempo para diferentes peers, partes que já tenham sido baixadas.

# Tracker

Sua resposabilidade é ajudar pares a encontrar outros pares.
Um tracker consiste de várias sessões torrent, a partir de cada sessão ele rastrea todos os pares participando de um determinado torrent.
Os pares contantam o tracker e o tracker responde com uma lista de pares que ele deve conectar-se.
Os pares só se conectam ao tracker quando iniciam ou em intervalos de tempos determinados.
O endereço do tracker é definido no arquivo torrent.
Trackers use a simple protocol layered on top of HTTP [3]. The tracker receives HTTP GET requests and it sends bencoded messages to the peer’s request. The tracker GET requests contain the following keys info_hash, peer_id, ip, port, uploaded, downloaded, left, and event.
- The info_hash key is how the tracker determines which torrent session the client is a part of or is joining.
- The peer_id is the id the client randomly generated at the start of the download.

# Arquivo torrent

O arquivo torrent é arquivo de metadados que representa a sessão do conteúdo que está sendo distribuído.
O arquivo torrent é criado com a URL do tracker e os identificadores do conteúdo que faz parte daquele torrent.
O torrent file é um dicionário que contém 2 chaves: announce e info.

* Announce é a URL do tracker. 
* Info é outro dicionário, com as seguintes chaves: name, piece length, pieces, e também lenght ou files key. 
    * name é uma string, que sugere o nome que será dado ao arquivo baixado.
    * piece length, mapeia o número de bytes que cada parte do arquivo vai ser dividida.
    * <del>pieces, é uma coleção de string de hash SHA1. É usado para confirmar o que está sendo baixado.
    * length ou files, caso seja só um arquivo length significa o tamanho em bytes do arquivo. <del>Caso seja um diretório, files mapeia uma lista de length e path name.
    