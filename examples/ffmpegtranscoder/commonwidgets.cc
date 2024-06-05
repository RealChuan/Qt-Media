#include "commonwidgets.hpp"

#include <QtWidgets>

namespace CommonWidgets {

auto createComboBox(QWidget *parent) -> QComboBox *
{
    const auto *comboBoxStyleSheet = "QComboBox {combobox-popup:0;}";
    auto *comboBox = new QComboBox(parent);
    comboBox->setView(new QListView(comboBox));
    comboBox->setMaxVisibleItems(10);
    comboBox->setStyleSheet(comboBoxStyleSheet);
    return comboBox;
}

} // namespace CommonWidgets
