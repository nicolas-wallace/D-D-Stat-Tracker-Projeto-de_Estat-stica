#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <ctime>
#include <chrono>
#include <map>

using namespace std;

bool diretorioExiste(const string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

void criarDiretorio(const string& path) {
#ifdef _WIN32
    system(("mkdir " + path).c_str());
#else
    system(("mkdir -p " + path).c_str());
#endif
}

void criarBackupArquivos() {
    cout << "Criando backup dos arquivos..." << endl;
    
    // Verificar se o diretório de backup existe, se não, criar
    if (!diretorioExiste("jogadores_bak")) {
        criarDiretorio("jogadores_bak");
        cout << "Diretorio 'jogadores_bak' criado." << endl;
    }
    
    // Lista de personagens para backup
    vector<string> personagens = {"Nicolle", "Jaeyk", "Thorne", "Riley", "Imugi", "Dean", "Mestre"};
    int arquivosSalvos = 0;
    
    for (const string& nome : personagens) {
        // Copiar arquivo de estatísticas
        string arquivoOriginal = "jogadores/" + nome + ".txt";
        string arquivoBackup = "jogadores_bak/" + nome + ".txt";
        
        ifstream origem(arquivoOriginal, ios::binary);
        if (origem.is_open()) {
            ofstream destino(arquivoBackup, ios::binary);
            destino << origem.rdbuf();
            destino.close();
            origem.close();
            arquivosSalvos++;
        }
        
        // Copiar arquivo de histórico
        arquivoOriginal = "jogadores/" + nome + "_historico.txt";
        arquivoBackup = "jogadores_bak/" + nome + "_historico.txt";
        
        origem.open(arquivoOriginal, ios::binary);
        if (origem.is_open()) {
            ofstream destino(arquivoBackup, ios::binary);
            destino << origem.rdbuf();
            destino.close();
            origem.close();
            arquivosSalvos++;
        }
    }
    
    if (arquivosSalvos > 0) {
        cout << "Backup concluído! " << arquivosSalvos << " arquivos foram copiados para 'jogadores_bak'." << endl;
    } else {
        cout << "Nenhum arquivo encontrado para backup." << endl;
    }
}

void limparTodosArquivos() {
    cout << "ATENÇÃO: Esta opção irá apagar todos os arquivos de histórico e estatísticas." << endl;
    cout << "Tem certeza que deseja continuar? (S/N): ";
    char confirmacao;
    cin >> confirmacao;
    confirmacao = toupper(confirmacao);
    
    if (confirmacao != 'S') {
        cout << "Operação cancelada." << endl;
        return;
    }

    criarBackupArquivos();
    
    // Lista de personagens para limpar arquivos
    vector<string> personagens = {"Nicolle", "Jaeyk", "Thorne", "Riley", "Imugi", "Dean", "Mestre"};
    
    for (const string& nome : personagens) {
        // Remove o arquivo de estatísticas
        string arquivoEstatisticas = "jogadores/" + nome + ".txt";
        remove(arquivoEstatisticas.c_str());
        
        // Remove o arquivo de histórico
        string arquivoHistorico = "jogadores/" + nome + "_historico.txt";
        remove(arquivoHistorico.c_str());
    }
    
    cout << "Todos os arquivos foram removidos com sucesso!" << endl;
}



double calcularDesvioPadrao(const vector<int>& rolagens, double media) {
    if (rolagens.size() <= 1) return 0.0;
    
    double somaDosQuadradosDasDiferencas = 0.0;
    for (int valor : rolagens) {
        double diferenca = valor - media;
        somaDosQuadradosDasDiferencas += diferenca * diferenca;
    }
    return sqrt(somaDosQuadradosDasDiferencas / rolagens.size());
}

double calcularMedia(const vector<int>& rolagens) {
    if (rolagens.empty()) return 0.0;
    
    double soma = 0.0;
    for (int valor : rolagens) {
        soma += valor;
    }
    return soma / rolagens.size();
}

vector<int> simularRolagens(int quantidadeRolagens, unsigned long long customSeed) {
    mt19937 gerador(customSeed);
    uniform_int_distribution<int> distribuicao(1, 20);
    
    cout << "Usando seed: " << customSeed << " para gerar rolagens" << endl;
    
    vector<int> resultados;
    for (int i = 0; i < quantidadeRolagens; i++) {
        resultados.push_back(distribuicao(gerador));
    }
    return resultados;
}

class Personagem {
public:
    string nome;
    double mediaDiaria;
    double mediaTotal;
    double desvioPadrao;
    double desvioPadraoDiario;
    double variancia;
    double varianciaTotal;
    vector<int> rolagens;
    vector<vector<int>> historicoRolagens; // Histórico de todas as sessões
    
    Personagem(string nome) {
        this->nome = nome;
        this->mediaDiaria = 0.0;
        this->mediaTotal = 0.0;
        this->desvioPadrao = 0.0;
        this->desvioPadraoDiario = 0.0;
        this->variancia = 0.0;
        this->varianciaTotal = 0.0;
        
        // Carrega dados anteriores se existirem
        carregarHistorico();
    }
    
    void setRolagem(const vector<int>& novaRolagem) {
        rolagens = novaRolagem;
        setMediaDiaria(novaRolagem);
        atualizarMediaTotal();
    }

    void setMediaDiaria(const vector<int>& novaRolagem) {
        mediaDiaria = calcularMedia(novaRolagem);
        desvioPadraoDiario = calcularDesvioPadrao(novaRolagem, mediaDiaria);
        variancia = desvioPadraoDiario * desvioPadraoDiario;
    }

    void atualizarMediaTotal() {
        if (rolagens.empty()) return;
        
        // Adiciona as novas rolagens ao histórico
        historicoRolagens.push_back(rolagens);
        
        // Combina todas as rolagens para cálculos
        vector<int> todasRolagens;
        for (const auto& sessao : historicoRolagens) {
            todasRolagens.insert(todasRolagens.end(), sessao.begin(), sessao.end());
        }
        
        // Calcula estatísticas com todas as rolagens
        mediaTotal = calcularMedia(todasRolagens);
        desvioPadrao = calcularDesvioPadrao(todasRolagens, mediaTotal);
        varianciaTotal = desvioPadrao * desvioPadrao;
    }
    
    map<int, double> calcularDistribuicaoNormal() {
        map<int, double> distribuicao;
        
        // Inicializa contagem de ocorrências para cada valor de 1 a 20
        for (int i = 1; i <= 20; ++i) {
            distribuicao[i] = 0;
        }
        
        // Conta as ocorrências de cada valor nas rolagens atuais
        for (int valor : rolagens) {
            distribuicao[valor]++;
        }
        
        // Converte contagens para frequências (porcentagens)
        int total = rolagens.size();
        if (total > 0) {
            for (auto& par : distribuicao) {
                par.second = (par.second / total) * 100.0;
            }
        }
        
        return distribuicao;
    }
    
    void carregarHistorico() {
        ifstream arquivo("jogadores/" + nome + "_historico.txt");
        if (!arquivo.is_open()) return; // Arquivo não existe ainda
        
        string linha;
        vector<int> sessaoRolagens;
        bool lendoRolagens = false;
        
        while (getline(arquivo, linha)) {
            if (linha.find("Sessão") != string::npos) {
                if (!sessaoRolagens.empty()) {
                    historicoRolagens.push_back(sessaoRolagens);
                    sessaoRolagens.clear();
                }
                lendoRolagens = true;
                continue;
            }
            
            if (lendoRolagens && linha.find("Rolagem") != string::npos) {
                size_t pos = linha.find_last_of(":");
                if (pos != string::npos) {
                    int valor = stoi(linha.substr(pos + 1));
                    sessaoRolagens.push_back(valor);
                }
            }
        }
        
        if (!sessaoRolagens.empty()) {
            historicoRolagens.push_back(sessaoRolagens);
        }
        
        arquivo.close();
    }
    
    void salvarHistorico() {
        ofstream arquivo("jogadores/" + nome + "_historico.txt");
        if (!arquivo.is_open()) {
            cerr << "O arquivo " << nome << "_historico.txt nao pode ser aberto." << endl;
            return;
        }
        
        arquivo << "Histórico de rolagens para " << nome << endl << endl;
        
        for (size_t i = 0; i < historicoRolagens.size(); ++i) {
            arquivo << "Sessão " << (i + 1) << ":" << endl;
            for (size_t j = 0; j < historicoRolagens[i].size(); ++j) {
                arquivo << "Rolagem " << (j + 1) << ": " << historicoRolagens[i][j] << endl;
            }
            arquivo << endl;
        }
        
        arquivo.close();
    }
    
    void processar() {
        ofstream arquivo("jogadores/" + nome + ".txt");
    
        if (!arquivo.is_open()) {
            cerr << "O arquivo " << nome << ".txt nao pode ser aberto." << endl;
            return;
        }
        
        map<int, double> distribuicao = calcularDistribuicaoNormal();
        
        arquivo << "Personagem: " << nome << endl << endl;
        
        // Estatísticas da sessão atual
        arquivo << "=== ESTATÍSTICAS DA SESSÃO ATUAL ===" << endl;
        arquivo << "Quantidade de rolagens: " << rolagens.size() << endl;
        arquivo << "Média de rolagem (sessão atual): " << mediaDiaria << endl;
        arquivo << "Desvio padrão (sessão atual): " << desvioPadraoDiario << endl;
        arquivo << "Variância (sessão atual): " << variancia << endl << endl;
        
        // Estatísticas totais (todas as sessões)
        arquivo << "=== ESTATÍSTICAS CUMULATIVAS ===" << endl;
        
        int totalRolagens = 0;
        for (const auto& sessao : historicoRolagens) {
            totalRolagens += sessao.size();
        }
        
        arquivo << "Total de sessões: " << historicoRolagens.size() << endl;
        arquivo << "Total de rolagens (todas as sessões): " << totalRolagens << endl;
        arquivo << "Média total (todas as sessões): " << mediaTotal << endl;
        arquivo << "Desvio padrão total: " << desvioPadrao << endl;
        arquivo << "Variância total: " << varianciaTotal << endl << endl;
        
        // Distribuição das rolagens atuais
        arquivo << "=== DISTRIBUIÇÃO DE ROLAGENS (SESSÃO ATUAL) ===" << endl;
        for (const auto& par : distribuicao) {
            arquivo << "Valor " << par.first << ": " << par.second << "%" << endl;
        }
        arquivo << endl;
        
        // Rolagens individuais da sessão atual
        arquivo << "=== ROLAGENS INDIVIDUAIS (SESSÃO ATUAL) ===" << endl;
        for (size_t i = 0; i < rolagens.size(); i++) {
            arquivo << "Rolagem " << (i + 1) << ": " << rolagens[i] << endl;
        }
        
        arquivo.close();
        cout << "Estatisticas de " << nome << " salvas com sucesso!" << endl;
        
        // Salva o histórico de todas as sessões
        salvarHistorico();
    }
};



int main() {
    if (!diretorioExiste("jogadores")) {
        criarDiretorio("jogadores");
        cout << "Diretorio 'jogadores' criado." << endl;
    }

    Personagem Nicolle("Nicolle");
    Personagem Jaeyk("Jaeyk");
    Personagem Thorne("Thorne");
    Personagem Riley("Riley");
    Personagem Imugi("Imugi");
    Personagem Dean("Dean");
    Personagem Mestre("Mestre");

    while (true) {
        int i = 0;
        char opcao;
        Personagem* selecionado = nullptr;
        vector<int> roladas;

        cout << "\nDigite qual personagem deseja operar:" << endl;
        cout << "N para Nicolle" << endl;
        cout << "T para Thorne" << endl;
        cout << "J para Jaeyk" << endl;
        cout << "R para Riley" << endl;
        cout << "I para Imugi" << endl;
        cout << "D para Dean" << endl;
        cout << "M para Mestre" << endl;
        cout << "S para simular rolagens aleatorias" << endl;
        cout << "L para LIMPAR TODOS OS ARQUIVOS" << endl;
        cout << "B para fazer um BACKUP" << endl;
        cout << "Q para sair" << endl;
        cin >> opcao;
        opcao = toupper(opcao);

        switch (opcao) {
            case 'N': selecionado = &Nicolle; break;
            case 'T': selecionado = &Thorne; break;
            case 'J': selecionado = &Jaeyk; break;
            case 'R': selecionado = &Riley; break;
            case 'I': selecionado = &Imugi; break;
            case 'D': selecionado = &Dean; break;
            case 'M': selecionado = &Mestre; break;
            case 'L': 
                limparTodosArquivos();
                continue;
            case 'B':
                criarBackupArquivos();
                continue;
            case 'S': {
                cout << "Qual personagem deseja simular? (N/T/J/R/I/D - A para TODOS): ";
                char personagemSim;
                cin >> personagemSim;
                personagemSim = toupper(personagemSim);

                vector<Personagem*> todos = {&Nicolle, &Thorne, &Jaeyk, &Riley, &Imugi, &Dean, &Mestre};
                
                switch (personagemSim) {
                    case 'N': selecionado = &Nicolle; break;
                    case 'T': selecionado = &Thorne; break;
                    case 'J': selecionado = &Jaeyk; break;
                    case 'R': selecionado = &Riley; break;
                    case 'I': selecionado = &Imugi; break;
                    case 'D': selecionado = &Dean; break;
                    case 'M': selecionado = &Mestre; break;
                    case 'A':
                        int quantidadeRolagens;
                        cout << "Quantas rolagens deseja simular? ";
                        cin >> quantidadeRolagens;

                        for (int j = 0; j < 7; j++) {
                            unsigned long long seed = chrono::high_resolution_clock::now().time_since_epoch().count();
                            roladas = simularRolagens(quantidadeRolagens, seed);
                            todos[j]->setRolagem(roladas);
                            todos[j]->processar();
                            continue;

                        }
                        continue;

                    default:
                        cout << "Personagem inválido." << endl;
                        continue;
                }
                
                int quantidadeRolagens;
                cout << "Quantas rolagens deseja simular? ";
                cin >> quantidadeRolagens;
                
                if (quantidadeRolagens <= 0) {
                    cout << "Quantidade inválida." << endl;
                    continue;
                }
                
                // Gera seed a partir do tempo atual
                unsigned long long seed = chrono::high_resolution_clock::now().time_since_epoch().count();
                roladas = simularRolagens(quantidadeRolagens, seed);
                
                selecionado->setRolagem(roladas);
                selecionado->processar();
                continue;
            }
            case 'Q': cout << "Encerrando programa." << endl; return 0;
            default:
                cout << "Opcao invalida." << endl;
                continue;
        }

        cout << "Iniciar registros de rolagem (1 a 20)" << endl;
        cout << "Digite qualquer coisa invalida para encerrar" << endl;

        while (true) {
            int valor;
            cout << "Rolagem " << (i + 1) << ": ";
            if (!(cin >> valor)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }

            if (valor < 1 || valor > 20) {
                cout << "Valor invalido. Digite entre 1 e 20." << endl;
                continue;
            }

            roladas.push_back(valor);
            i++;
        }

        selecionado->setRolagem(roladas);
        selecionado->processar();
    }

    return 0;
}