/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetapptest.h"
#include "shortcutmanager.h"

class TstNewWindowAction : public QObject
{
	Q_OBJECT

	private slots:
		void testNewWindowActionRegistration()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);

			QAction *action = ShortcutManager::instance().action(
				QStringLiteral("diagrameditor.new_window"), editor);
			QVERIFY(action);
			QCOMPARE(action->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
			QVERIFY(action->isEnabled());
		}

		void testTriggeringActionOpensSecondWindow()
		{
			QETDiagramEditor *first_editor = QetAppTest::editor();
			QVERIFY(first_editor);
			QCOMPARE(QETApp::diagramEditors().size(), 1);

			QAction *action = ShortcutManager::instance().action(
				QStringLiteral("diagrameditor.new_window"), first_editor);
			QVERIFY(action);

			action->trigger();
			QTRY_COMPARE(QETApp::diagramEditors().size(), 2);

			QList<QETDiagramEditor *> editors = QETApp::diagramEditors();
			QCOMPARE(editors.size(), 2);
			QETDiagramEditor *second_editor = (editors.first() == first_editor)
				? editors.last()
				: editors.first();
			QVERIFY(second_editor && second_editor != first_editor);

			QVERIFY(second_editor->close());
			QTRY_COMPARE(QETApp::diagramEditors().size(), 1);
		}
};

QET_APP_TEST_MAIN(TstNewWindowAction)
#include "tst_newwindowaction.moc"
