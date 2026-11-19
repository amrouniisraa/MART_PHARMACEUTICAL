# MART PHARMACEUTICAL

Système intelligent de gestion des stocks et de production pharmaceutique.

## Présentation

MART PHARMACEUTICAL automatise et optimise la gestion des stocks et de la production dans une entreprise pharmaceutique grâce à l'IoT : identification des produits par RFID, mesure des quantités par balances électroniques, et traitement des données dans une base Oracle.

## Objectifs

- Automatiser la gestion des stocks pharmaceutiques
- Identifier les produits par tags RFID
- Contrôler les quantités avec des balances électroniques (tolérance ±0,5 %)
- Assurer la traçabilité des matières premières et des produits finis
- Gérer et suivre la production
- Réduire les erreurs humaines
- Générer des rapports et des statistiques

## Architecture

| Couche | Contenu |
|--------|---------|
| Matériel | ESP32, lecteur RFID RC522, tags RFID, balances, capteurs |
| Communication | Wi-Fi, Firebase Realtime Database, JSON, REST API |
| Base de données | Oracle SQL / PL/SQL |
| Application web | Oracle APEX, ORDS |

### Flux des données

Produit → Tag RFID → Lecteur RFID → ESP32 (+ balance) → Wi-Fi → Firebase / ORDS → Oracle SQL → Oracle APEX → Interface web

## Structure du dépôt

- `esp32/mart_pharma.ino` : code de l'ESP32 (lecture RFID, envoi vers Firebase et Oracle APEX)
- `database/schema.sql` : script de création de la base Oracle
- `README.md` : présentation du projet

## Câblage ESP32 et RC522

| RC522 | ESP32 |
|-------|-------|
| SDA (SS) | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| 3.3V | 3V3 |
| GND | GND |

## Technologies

ESP32, RFID, balance électronique, Wi-Fi, Firebase, JSON, REST API, ORDS, Oracle SQL, PL/SQL, Oracle APEX

## Utilisateurs et rôles

- Administrateur
- Responsable de stock
- Responsable de production
- Pharmacien / Responsable qualité
- Opérateur
- Responsable (consultation des rapports)

## Base de données

Le script de création se trouve dans `database/schema.sql` (Oracle SQL).

Tables : FOURNISSEUR, UTILISATEUR, MATIERE_PREMIERE, COMMANDE_FOURNISSEUR, LOT, MOUVEMENT_STOCK, PRODUIT_PHARMACEUTIQUES, PRODUCTION, RFID_SCANS.

La table RFID_SCANS reçoit les scans envoyés par l'ESP32 (device_id, uid, type de carte, date du scan) via un service REST ORDS.

## Installation

1. Créer les tables en exécutant `database/schema.sql` dans Oracle APEX (SQL Workshop).
2. Créer le service REST ORDS qui insère les scans dans RFID_SCANS.
3. Dans `esp32/mart_pharma.ino`, remplacer les valeurs factices (Wi-Fi, URL Firebase, URL Oracle APEX) par vos propres identifiants.
4. Téléverser le code sur l'ESP32 avec Arduino IDE.

## Feuille de route

1. Finaliser le MCD
2. Créer les tables Oracle SQL
3. Définir les relations et contraintes
4. Configurer Firebase
5. Programmer l'ESP32 et le RFID
6. Créer les services REST avec ORDS
7. Connecter l'ESP32 à Oracle Database
8. Développer l'application Oracle APEX
9. Gérer les utilisateurs et les rôles
10. Tester et valider le système

## Auteur

amrouniisraa
