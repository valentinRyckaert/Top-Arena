#pragma once

#include <QWidget>

class QLabel;

namespace toupie {

/// Aide : récapitulatif des touches et de la souris (d'après les paramètres).
class HelpPage : public QWidget
{
    Q_OBJECT

public:
    explicit HelpPage(QWidget *parent = nullptr);

    /// Régénère le récapitulatif avec les touches actuelles.
    void refresh();
    /// Licence du jeu, composants tiers et textes complets de leurs licences.
    static QString licensesText();

signals:
    void backRequested();
    void settingsRequested();

private:
    void showLicenses();

    QLabel *m_text = nullptr;
};

} // namespace toupie
