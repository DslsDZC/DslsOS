# Documentation DslsOS

[English](README.md) / [简体中文](README_CN.md) / [日本語](README_JP.md) / **Français** / [Deutsch](README_DE.md)

[![Static Badge](https://img.shields.io/badge/License_GPLv3-0?logo=gnu&color=8A2BE2)](https://github.com/DslsDZC/DslsOS/blob/main/LICENSE.txt)
[![Discord](https://img.shields.io/discord/1423859793101328386?logo=discord&labelColor=%20%235462eb&logoColor=%20%23f5f5f5&color=%20%235462eb)](https://discord.gg/xz5pEK7XRR)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/DslsDZC/DslsOS)

## Aperçu

DslsOS est un système d'exploitation moderne doté d'une architecture micro-noyau avec des capacités avancées de calcul distribué. Conçu pour fournir des environnements d'exécution efficaces, sécurisés et flexibles pour divers scénarios informatiques, il combine la stabilité de la conception micro-noyau avec la puissance des systèmes distribués.

## Fonctionnalités principales

### 🏗️ Conception Micro-noyau
- Composants à privilèges minimaux dans l'espace noyau
- Services principaux (gestion des processus, allocation mémoire, IPC) implémentés comme composants modulaires
- Stabilité système améliorée et surface d'attaque réduite

### 🌐 Support du Calcul Distribué
- Capacités natives de gestion de cluster
- Coordination automatique des nœuds et déploiement de services
- Support des appareils embarqués aux systèmes distribués à grande échelle

### ⚡ Ordonnancement Intelligent
- Ordonnancement dynamique des tâches basé sur la charge système
- Algorithmes d'ordonnancement multiples pour une utilisation optimale des ressources
- Adaptation des performances en temps réel

### 🔒 Architecture de Sécurité
- Modèle de sécurité Zero Trust
- Authentification, autorisation et chiffrement intégrés
- Journalisation d'audit et surveillance complètes

## Implémentation du Calcul Distribué

### Architecture Cœur

**Module de Gestion Distribuée** (`distributed_management.c`):
- Structures `NODE_INFO`, `CLUSTER_INFO`, `SERVICE_INFO` pour la modélisation système
- APIs `DmCreateCluster`, `DmJoinCluster`, `DmCreateService` pour la gestion du cycle de vie du cluster

### Mécanismes Clés

#### 1. Gestion des Nœuds et Clusters
- Découverte automatique des nœuds via le mécanisme de découverte de services
- Surveillance par heartbeat pour le suivi de l'état des nœuds
- Support des types de nœuds spécialisés (calcul, stockage, etc.)

#### 2. Déploiement de Services
- Déploiement de services basé sur des conteneurs utilisant la virtualisation légère
- Mise à l'échelle dynamique des services avec l'interface `DmScaleService`
- Orchestration de services prenant en compte les dépendances

#### 3. Répartition de Charge
- Algorithmes multiples : Round Robin, Moins de Connexions, Hachage IP
- Ordonnancement tenant compte des ressources en temps réel
- Migration automatique des tâches basée sur la charge des nœuds

#### 4. Haute Disponibilité
- Détection et récupération automatiques des pannes
- Basculement des services vers les nœuds de sauvegarde
- Capacités d'auto-guérison du cluster

## Architecture du Système

### Conception en Couches
- **Couche Cœur** : Micro-noyau gérant les processus, la mémoire, l'IPC
- **Couche Service** : Composants modulaires (gestion distribuée, conteneurs, sécurité)
- **Couche Matérielle** : Abstraction matérielle pour la compatibilité multiplateforme

### Composants Clés

**Système de Fichiers** (`dslsfs.c`):
- Stockage distribué sur plusieurs nœuds
- Réplication et mise en cache automatiques des données
- Opérations transactionnelles avec journalisation

**Système de Conteneurs** (`container_system.c`):
- Virtualisation légère avec isolation des processus
- Limites de ressources et surveillance
- Réseau et stockage des conteneurs

**Système de Sécurité** (`security_architecture.c`):
- Authentification multi-facteurs
- Contrôle d'accès basé sur les rôles
- Chiffrement de bout en bout

## Écosystème de Développement

### Construction et Outils
- Construction multiplateforme avec CMake
- Support des compilateurs GCC, Clang
- Modes de compilation Debug et Release

### Standards de Codage
- Conventions de codage unifiées
- Documentation compatible Doxygen
- Processus de contribution structuré

## Public Cible

- **Développeurs de Noyau** : Conception et optimisation des modules cœur
- **Développeurs d'Applications** : Développement de logiciels utilisant les APIs système
- **Administrateurs Système** : Gestion des environnements de déploiement
- **Chercheurs en Sécurité** : Analyse de la sécurité du système

## Licence Open Source

Publié sous GNU General Public License v3 (GPLv3), garantissant la liberté d'utilisation, de modification et de distribution du logiciel.

---

## ⭐ Contribution

1. Forkez ce dépôt
2. Créez votre branche (`git checkout -b feature/AmazingFeature`)
3. Committez vos changements (`git commit -m 'Ajouter une fonctionnalité incroyable'`)
4. Pushez vers la branche (`git push origin feature/AmazingFeature`)
5. Ouvrez une Pull Request

---

## 🌟 Remerciements

Remerciements à tous les développeurs qui ont contribué au code, à la documentation et aux suggestions pour ce projet !

<p align="center">
  <a href="https://github.com/DslsDZC/DslsOS/graphs/contributors">
    <img src="https://contrib.rocks/image?repo=DslsDZC/DslsOS" alt="Contributeurs">
  </a>
</p>

---

## Star

[![Star History Chart](https://api.star-history.com/svg?repos=DslsDZC/DslsOS&type=Date)](https://star-history.com/#DslsDZC/DslsOS&Date)
![](https://repobeats.axiom.co/api/embed/035bf80bf2a7c84a9e7fc079d4b8b446edce7d1c.svg)

---

*Que ce soit pour construire des systèmes embarqués, déployer des clusters distribués ou développer des applications personnalisées, DslsOS fournit des bases techniques fiables et des capacités d'extension flexibles.*
