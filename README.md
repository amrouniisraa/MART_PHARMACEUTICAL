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
| Matériel | ESP32, lecteurs RFID, tags RFID, balances, capteurs |
| Communication | Wi-Fi, Firebase Realtime Database, JSON, REST API |
| Base de données | Oracle SQL / PL/SQL |
| Application web | Oracle APEX, ORDS |

### Flux des données

Produit → Tag RFID → Lecteur RFID → ESP32 (+ balance) → Wi-Fi → Firebase / ORDS → Oracle SQL → Oracle APEX → Interface web

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

Tables principales : USERS, ROLES, PRODUCTS, SUPPLIERS, WAREHOUSES, RAW_MATERIALS, STOCK, STOCK_MOVEMENTS, PRODUCTION_ORDERS, PRODUCTION_DETAILS, RFID_READERS, RFID_SCANS, QUALITY_CONTROL, LOTS.

La relation entre PRODUCTION_ORDERS et RAW_MATERIALS est de type N:N, gérée par PRODUCTION_DETAILS.

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
