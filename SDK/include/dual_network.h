/**
 * @file dual_network.h
 * @brief Module réseau de libdual : requêtes HTTP(S) pour l'intégration à
 *        l'eShop de la console, et sockets locaux pour le jeu multijoueur
 *        en réseau local.
 *
 * Ce module expose deux familles de fonctionnalités distinctes :
 * - Les requêtes HTTP (DUAL_HttpRequest) : utilisées pour communiquer avec
 *   les services en ligne de la console (boutique, classements, DLC).
 * - Les sockets locaux (DUAL_LocalSocket) : utilisées pour le jeu multijoueur
 *   en réseau local entre plusieurs consoles, sans passer par Internet.
 *
 * @note Toutes les opérations réseau de ce module sont asynchrones : elles
 *       retournent immédiatement un handle, et l'avancement doit être
 *       interrogé à chaque frame via les fonctions de Poll().
 */

#ifndef DUAL_NETWORK_H
#define DUAL_NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include "dual_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Types opaques
 * ========================================================================== */

/**
 * @brief Gestionnaire réseau global, responsable de l'exécution asynchrone
 *        des requêtes HTTP et de la gestion des sockets locaux.
 */
typedef struct DUAL_NetworkManager DUAL_NetworkManager;

/**
 * @brief Représente une requête HTTP(S) asynchrone en cours ou terminée.
 */
typedef struct DUAL_HttpRequest DUAL_HttpRequest;

/**
 * @brief Représente une socket de communication locale, utilisée pour le
 *        multijoueur en réseau local (host ou client).
 */
typedef struct DUAL_LocalSocket DUAL_LocalSocket;

/* ============================================================================
 *  Enumérations
 * ========================================================================== */

/**
 * @brief Méthode HTTP utilisée pour une requête.
 */
typedef enum DUAL_HttpMethod {
    DUAL_HTTP_GET    = 0, /**< Requête de récupération de données. */
    DUAL_HTTP_POST   = 1, /**< Requête d'envoi de données (création). */
    DUAL_HTTP_PUT    = 2, /**< Requête de remplacement de données. */
    DUAL_HTTP_DELETE = 3  /**< Requête de suppression de données. */
} DUAL_HttpMethod;

/**
 * @brief État d'avancement d'une requête HTTP asynchrone.
 */
typedef enum DUAL_HttpStatus {
    DUAL_HTTP_STATUS_EN_COURS  = 0, /**< La requête est toujours en cours d'exécution. */
    DUAL_HTTP_STATUS_TERMINEE  = 1, /**< La requête s'est terminée avec succès (code HTTP 2xx). */
    DUAL_HTTP_STATUS_ERREUR    = 2  /**< La requête a échoué (erreur réseau ou code HTTP 4xx/5xx). */
} DUAL_HttpStatus;

/**
 * @brief Rôle d'une socket locale au sein d'une session multijoueur.
 */
typedef enum DUAL_SocketRole {
    DUAL_SOCKET_ROLE_HOTE   = 0, /**< La socket héberge la session et accepte des connexions entrantes. */
    DUAL_SOCKET_ROLE_CLIENT = 1  /**< La socket se connecte à une session hébergée par une autre console. */
} DUAL_SocketRole;

/* ============================================================================
 *  Structures
 * ========================================================================== */

/**
 * @brief En-tête HTTP clé-valeur, utilisé pour construire une liste d'en-têtes
 *        personnalisés sur une requête.
 */
typedef struct DUAL_HttpHeader {
    const char* cle;    /**< Nom de l'en-tête HTTP (ex: "Content-Type"). */
    const char* valeur; /**< Valeur associée à l'en-tête. */
} DUAL_HttpHeader;

/**
 * @brief Paramètres complets de configuration d'une requête HTTP.
 */
typedef struct DUAL_HttpRequestParams {
    const char*            url;             /**< URL complète de la requête (incluant le schéma https://). */
    DUAL_HttpMethod         methode;         /**< Méthode HTTP à utiliser. */
    const DUAL_HttpHeader*  en_tetes;        /**< Tableau d'en-têtes HTTP additionnels (peut être NULL). */
    int32_t                 nombre_en_tetes; /**< Nombre d'éléments dans le tableau en_tetes. */
    const uint8_t*          corps;           /**< Corps de la requête, pour POST/PUT (peut être NULL). */
    uint64_t                taille_corps;    /**< Taille du corps de la requête, en octets. */
    int32_t                 timeout_ms;      /**< Délai maximal d'attente avant échec, en millisecondes. */
} DUAL_HttpRequestParams;

/**
 * @brief Informations sur une console pair détectée sur le réseau local,
 *        disponible pour rejoindre une session multijoueur.
 */
typedef struct DUAL_LocalPeerInfo {
    char    nom_joueur[32]; /**< Nom affiché du joueur hébergeant la session détectée. */
    char    adresse_ip[46]; /**< Adresse IP de la console pair (compatible IPv4 et IPv6). */
    int32_t port;           /**< Port réseau utilisé par la session détectée. */
} DUAL_LocalPeerInfo;

/* ============================================================================
 *  Cycle de vie du gestionnaire réseau
 * ========================================================================== */

/**
 * @brief Initialise le sous-système réseau de libdual.
 *
 * @param app Instance de l'application libdual.
 * @param out_network Pointeur recevant le gestionnaire réseau créé en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_NetworkManager_Create(DUAL_App* app, DUAL_NetworkManager** out_network);

/**
 * @brief Détruit le gestionnaire réseau, annulant toutes les requêtes et
 *        fermant toutes les sockets encore actives.
 *
 * @param network Gestionnaire réseau à détruire.
 */
void DUAL_NetworkManager_Destroy(DUAL_NetworkManager* network);

/**
 * @brief Indique si une connexion Internet est actuellement disponible.
 *
 * @param network Gestionnaire réseau.
 * @return true si une connexion Internet est détectée, false sinon.
 */
bool DUAL_NetworkManager_IsInternetAvailable(const DUAL_NetworkManager* network);

/* ============================================================================
 *  Requêtes HTTP (intégration eShop)
 * ========================================================================== */

/**
 * @brief Démarre une nouvelle requête HTTP asynchrone.
 *
 * La requête s'exécute en arrière-plan ; son avancement doit être surveillé
 * via DUAL_HttpRequest_Poll() à chaque frame.
 *
 * @param network Gestionnaire réseau.
 * @param params Paramètres complets de la requête à exécuter.
 * @param out_request Pointeur recevant le handle de la requête créée.
 * @return DUAL_OK si la requête a été correctement mise en file d'attente,
 *         ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_HttpRequest_Start(DUAL_NetworkManager* network,
                                    const DUAL_HttpRequestParams* params,
                                    DUAL_HttpRequest** out_request);

/**
 * @brief Interroge l'état d'avancement d'une requête HTTP en cours.
 *
 * @param request Requête à interroger.
 * @return État courant de la requête (en cours, terminée, ou en erreur).
 */
DUAL_HttpStatus DUAL_HttpRequest_Poll(const DUAL_HttpRequest* request);

/**
 * @brief Récupère le code de statut HTTP retourné par le serveur, une fois la
 *        requête terminée.
 *
 * @param request Requête terminée.
 * @return Code de statut HTTP (ex: 200, 404, 500), ou 0 si la requête n'est pas terminée.
 */
int32_t DUAL_HttpRequest_GetStatusCode(const DUAL_HttpRequest* request);

/**
 * @brief Récupère un pointeur vers le corps de la réponse HTTP reçue.
 *
 * Le pointeur retourné reste valide jusqu'à la destruction de la requête via
 * DUAL_HttpRequest_Destroy().
 *
 * @param request Requête terminée.
 * @param out_taille_octets Pointeur recevant la taille du corps de la réponse, en octets.
 * @return Pointeur vers les données du corps de la réponse, ou NULL si la requête n'est pas terminée.
 */
const uint8_t* DUAL_HttpRequest_GetResponseBody(const DUAL_HttpRequest* request,
                                                 uint64_t* out_taille_octets);

/**
 * @brief Annule une requête HTTP en cours d'exécution.
 *
 * @param request Requête à annuler.
 */
void DUAL_HttpRequest_Cancel(DUAL_HttpRequest* request);

/**
 * @brief Détruit une requête HTTP et libère la mémoire associée à sa réponse.
 *
 * @param request Requête à détruire.
 */
void DUAL_HttpRequest_Destroy(DUAL_HttpRequest* request);

/* ============================================================================
 *  Sockets locales (multijoueur réseau local)
 * ========================================================================== */

/**
 * @brief Crée et démarre une socket locale, en tant qu'hôte ou client.
 *
 * En mode hôte, la socket commence immédiatement à écouter les connexions
 * entrantes sur le port spécifié. En mode client, la socket tente de se
 * connecter à l'adresse fournie.
 *
 * @param network Gestionnaire réseau.
 * @param role Rôle de la socket (hôte ou client).
 * @param adresse_ip Adresse IP cible (mode client) ou adresse d'écoute (mode hôte, peut être NULL pour toutes les interfaces).
 * @param port Port réseau à utiliser.
 * @param out_socket Pointeur recevant la socket créée en cas de succès.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_LocalSocket_Create(DUAL_NetworkManager* network, DUAL_SocketRole role,
                                     const char* adresse_ip, int32_t port,
                                     DUAL_LocalSocket** out_socket);

/**
 * @brief Ferme une socket locale et libère ses ressources.
 *
 * @param socket Socket à fermer.
 */
void DUAL_LocalSocket_Close(DUAL_LocalSocket* socket);

/**
 * @brief Recherche les consoles pairs annonçant une session multijoueur
 *        disponible sur le réseau local.
 *
 * @param network Gestionnaire réseau.
 * @param out_peers Tableau pré-alloué recevant les informations des pairs détectés.
 * @param taille_max_tableau Nombre maximal d'éléments que le tableau out_peers peut contenir.
 * @param out_nombre_peers_trouves Pointeur recevant le nombre réel de pairs détectés.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_LocalSocket_DiscoverPeers(DUAL_NetworkManager* network,
                                            DUAL_LocalPeerInfo* out_peers,
                                            int32_t taille_max_tableau,
                                            int32_t* out_nombre_peers_trouves);

/**
 * @brief Envoie un message de données sur une socket locale connectée.
 *
 * @param socket Socket connectée par laquelle envoyer les données.
 * @param donnees Pointeur vers les données à envoyer.
 * @param taille_octets Taille des données à envoyer, en octets.
 * @return DUAL_OK en cas de succès, ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_LocalSocket_Send(DUAL_LocalSocket* socket, const uint8_t* donnees,
                                   uint64_t taille_octets);

/**
 * @brief Tente de recevoir un message de données depuis une socket locale,
 *        sans bloquer si aucune donnée n'est disponible.
 *
 * @param socket Socket connectée depuis laquelle recevoir les données.
 * @param out_buffer Buffer destination recevant les données reçues.
 * @param taille_buffer Taille disponible du buffer destination, en octets.
 * @param out_octets_recus Pointeur recevant le nombre réel d'octets reçus (0 si aucune donnée disponible).
 * @return DUAL_OK en cas de succès (même si aucune donnée n'a été reçue), ou un code d'erreur DUAL_Result sinon.
 */
DUAL_Result DUAL_LocalSocket_Receive(DUAL_LocalSocket* socket, uint8_t* out_buffer,
                                      uint64_t taille_buffer, uint64_t* out_octets_recus);

/**
 * @brief Indique si une socket locale est actuellement connectée à un pair.
 *
 * @param socket Socket à interroger.
 * @return true si la socket est connectée, false sinon.
 */
bool DUAL_LocalSocket_IsConnected(const DUAL_LocalSocket* socket);

#ifdef __cplusplus
}
#endif

#endif /* DUAL_NETWORK_H */
