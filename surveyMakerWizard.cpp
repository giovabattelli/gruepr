#include "surveyMakerWizard.h"
#include "gruepr_globals.h"
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRadioButton>
#include <QSettings>

//implement export functionality
//make multichoice sample questions dialog
//finish load functionaloity: preload student names into ui
//make upload roster ui
//crash when loading test.gru then springproj.gru--doesn't crash when disabling loading of multichoice questions

SurveyMakerWizard::SurveyMakerWizard(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Make a survey"));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumWidth(800);
    setMinimumHeight(600);

    QSettings savedSettings;
    restoreGeometry(savedSettings.value("surveyMakerWindowGeometry").toByteArray());
    saveFileLocation.setFile(savedSettings.value("surveyMakerSaveFileLocation", "").toString());

    auto palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::Mid, palette.color(QPalette::Base));
    setPalette(palette);

    setPage(Page::introtitle, new IntroPage);
    setPage(Page::demographics, new DemographicsPage);
    setPage(Page::multichoice, new MultipleChoicePage);
    setPage(Page::schedule, new SchedulePage);
    setPage(Page::courseinfo, new CourseInfoPage);
    setPage(Page::previewexport, new PreviewAndExportPage);

    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::CustomButton1 << QWizard::BackButton << QWizard::NextButton;
    setButtonLayout(buttonLayout);

    button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: #" DEEPWATERHEX "; "));
    setButtonText(QWizard::CancelButton, "\u00AB  " + tr("Home"));
    button(QWizard::CustomButton1)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: #" DEEPWATERHEX "; "));
    setButtonText(QWizard::CustomButton1, tr("Load Previous Survey"));
    connect(this, &QWizard::customButtonClicked, this, &SurveyMakerWizard::loadSurvey);
    button(QWizard::BackButton)->setStyleSheet(STDBUTTONSTYLE);
    setButtonText(QWizard::BackButton, "\u2B60  " + tr("Previous Step"));
    setOption(QWizard::NoBackButtonOnStartPage);
    button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    setButtonText(QWizard::NextButton, tr("Next Step") + "  \u2B62");
}

SurveyMakerWizard::~SurveyMakerWizard()
{
    QSettings savedSettings;
    savedSettings.setValue("surveyMakerWindowGeometry", saveGeometry());
    savedSettings.setValue("surveyMakerSaveFileLocation", saveFileLocation.canonicalFilePath());
}

void SurveyMakerWizard::invalidExpression(QWidget *textWidget, QString &currText, QWidget *parent)
{
    auto *lineEdit = qobject_cast<QLineEdit *>(textWidget);
    if(lineEdit != nullptr)
    {
        lineEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/'));
    }

    auto *textEdit = qobject_cast<QTextEdit *>(textWidget);
    if(textEdit != nullptr)
    {
        if(textEdit != nullptr)
        {
            textEdit->setText(currText.remove(',').remove('&').remove('<').remove('>').remove('/').remove('\n').remove('\r'));
        }
    }

    QApplication::beep();
    QMessageBox::warning(parent, tr("Format error"), tr("Sorry, the following punctuation is not allowed:\n"
                                                        "    ,  &  <  > / {enter} \n"
                                                        "Other punctuation is allowed."));
}

void SurveyMakerWizard::loadSurvey(int customButton)
{
    //make sure we got here with custombutton1
    if(customButton != QWizard::CustomButton1)
    {
        return;
    }

    //read all options from a text file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), saveFileLocation.canonicalFilePath(), tr("gruepr survey File (*.gru);;All Files (*)"));
    if( !(fileName.isEmpty()) )
    {
        QFile loadFile(fileName);
        if(loadFile.open(QIODevice::ReadOnly))
        {
            saveFileLocation.setFile(QFileInfo(fileName).canonicalPath());
            QJsonDocument loadDoc(QJsonDocument::fromJson(loadFile.readAll()));
            QJsonObject loadObject = loadDoc.object();

            if(loadObject.contains("Title") && loadObject["Title"].isString())
            {
                setField("SurveyTitle", loadObject["Title"].toString());
            }
            if(loadObject.contains("FirstName") && loadObject["FirstName"].isBool())
            {
                setField("FirstName", loadObject["FirstName"].toBool());
            }
            if(loadObject.contains("LastName") && loadObject["LastName"].isBool())
            {
                setField("LastName", loadObject["LastName"].toBool());
            }
            if(loadObject.contains("Email") && loadObject["Email"].isBool())
            {
                setField("Email", loadObject["Email"].toBool());
            }
            if(loadObject.contains("Gender") && loadObject["Gender"].isBool())
            {
                setField("Gender", loadObject["Gender"].toBool());
            }
            if(loadObject.contains("GenderType") && loadObject["GenderType"].isDouble())
            {
                setField("genderOptions", loadObject["GenderType"].toInt());
            }
            if(loadObject.contains("URM") && loadObject["URM"].isBool())
            {
                setField("RaceEthnicity", loadObject["URM"].toBool());
            }

            if(loadObject.contains("numAttributes") && loadObject["numAttributes"].isDouble())
            {
                int numMultiChoiceQuestions = loadObject["numAttributes"].toInt();
                setField("multiChoiceNumQuestions", numMultiChoiceQuestions);   // need to set the number before setting the texts, responses, or multis
                QList<QString> multiChoiceQuestionTexts;
                QList<QList<QString>> multiChoiceQuestionResponses;
                QList<bool> multiChoiceQuestionMultis;
                auto responseOptions = QString(RESPONSE_OPTIONS).split(';');
                for(int question = 0; question < numMultiChoiceQuestions; question++)
                {
                    multiChoiceQuestionTexts << "";
                    if(loadObject.contains("Attribute" + QString::number(question+1)+"Question") &&
                        loadObject["Attribute" + QString::number(question+1)+"Question"].isString())
                    {
                        multiChoiceQuestionTexts.last() = loadObject["Attribute" + QString::number(question+1)+"Question"].toString();
                    }

                    multiChoiceQuestionResponses << QStringList({""});
                    if(loadObject.contains("Attribute" + QString::number(question+1)+"Response") &&
                        loadObject["Attribute" + QString::number(question+1)+"Response"].isDouble())
                    {
                        auto responseIndex = loadObject["Attribute" + QString::number(question+1)+"Response"].toInt() - 1;  // "-1" for historical reasons :(
                        if((responseIndex >= 0) && (responseIndex < responseOptions.size()-1))
                        {
                            multiChoiceQuestionResponses.last() = responseOptions[responseIndex].split(" / ");
                        }
                        else if(loadObject.contains("Attribute" + QString::number(question+1)+"Options") &&
                             loadObject["Attribute" + QString::number(question+1)+"Options"].isString())
                        {
                            multiChoiceQuestionResponses.last() = loadObject["Attribute" + QString::number(question+1)+"Options"].toString().split(" / ");
                        }
                    }
                    multiChoiceQuestionMultis << false;
                    if(loadObject.contains("Attribute" + QString::number(question+1)+"AllowMultiResponse") &&
                        loadObject["Attribute" + QString::number(question+1)+"AllowMultiResponse"].isBool())
                    {
                        multiChoiceQuestionMultis.last() = loadObject["Attribute" + QString::number(question+1)+"AllowMultiResponse"].toBool();
                    }
                }
                setField("multiChoiceQuestionTexts", multiChoiceQuestionTexts);
                setField("multiChoiceQuestionResponses", QVariant::fromValue<QList<QList<QString>>>(multiChoiceQuestionResponses));
                setField("multiChoiceQuestionMultis", QVariant::fromValue<QList<bool>>(multiChoiceQuestionMultis));
            }

            if(loadObject.contains("Timezone") && loadObject["Timezone"].isBool())
            {
                setField("Timezone", loadObject["Timezone"].toBool());
            }
            if(loadObject.contains("Schedule") && loadObject["Schedule"].isBool())
            {
                setField("Schedule", loadObject["Schedule"].toBool());
            }
            if(loadObject.contains("ScheduleAsBusy") && loadObject["ScheduleAsBusy"].isBool())
            {
                setField("scheduleBusyOrFree", loadObject["ScheduleAsBusy"].toBool()? SchedulePage::busy : SchedulePage::free);
            }
            QStringList scheduleDayNames;
            for(int day = 0; day < MAX_DAYS; day++)
            {
                QString dayString1 = "scheduleDay" + QString::number(day+1);
                QString dayString2 = dayString1 + "Name";
                if(loadObject.contains(dayString2) && loadObject[dayString2].isString())
                {
                    scheduleDayNames << loadObject[dayString2].toString();
                    //older style was to include day name AND a bool that could turn off the day
                    if(loadObject.contains(dayString1) && loadObject[dayString1].isBool())
                    {
                        if(!loadObject[dayString1].toBool())
                        {
                            scheduleDayNames.last() = "";
                        }
                    }
                }
                else
                {
                    scheduleDayNames << "";
                    if(loadObject.contains(dayString1) && loadObject[dayString1].isBool())
                    {
                        if(loadObject[dayString1].toBool())
                        {
                            scheduleDayNames.last() = defaultDayNames.at(day);
                        }
                    }
                }
            }
            setField("scheduleDayNames", scheduleDayNames);
            if(loadObject.contains("scheduleStartHour") && loadObject["scheduleStartHour"].isDouble())
            {
                setField("scheduleFrom", loadObject["scheduleStartHour"].toInt());
            }
            if(loadObject.contains("scheduleEndHour") && loadObject["scheduleEndHour"].isDouble())
            {
                setField("scheduleTo", loadObject["scheduleEndHour"].toInt());
            }
            if(loadObject.contains("baseTimezone") && loadObject["baseTimezone"].isString())
            {
                setField("baseTimezone", loadObject["baseTimezone"].toString());
            }
            if(loadObject.contains("scheduleQuestion") && loadObject["scheduleQuestion"].isString())
            {
                setField("ScheduleQuestion", loadObject["scheduleQuestion"].toString());
            }
            else
            {
                setField("ScheduleQuestion", SchedulePage::generateScheduleQuestion(field("scheduleBusyOrFree").toInt() == SchedulePage::busy,
                                                                                    field("Timezone").toBool(),
                                                                                    field("baseTimezone").toString()));
            }

            if(loadObject.contains("Section") && loadObject["Section"].isBool())
            {
                setField("Section", loadObject["Section"].toBool());
            }
            if(loadObject.contains("SectionNames") && loadObject["SectionNames"].isString())
            {
                setField("SectionNames", loadObject["SectionNames"].toString().split(','));
            }

            if(loadObject.contains("PreferredTeammates") && loadObject["PreferredTeammates"].isBool())
            {
                setField("PrefTeammate", loadObject["PreferredTeammates"].toBool());
            }
            if(loadObject.contains("PreferredNonTeammates") && loadObject["PreferredNonTeammates"].isBool())
            {
                setField("PrefNonTeammate", loadObject["PreferredNonTeammates"].toBool());
            }
            if(loadObject.contains("numPrefTeammates") && loadObject["numPrefTeammates"].isDouble())
            {
                setField("numPrefTeammates", loadObject["numPrefTeammates"].toInt());
            }

            loadFile.close();
        }
        else
        {
            QMessageBox::critical(this, tr("File Error"), tr("This file cannot be read."));
        }
    }
}


SurveyMakerPage::SurveyMakerPage(SurveyMakerWizard::Page page, QWidget *parent)
    : QWizardPage(parent),
    numQuestions(SurveyMakerWizard::numOfQuestionsInPage[page])
{
    QString title = "<span style=\"color: #" DEEPWATERHEX "\">";
    for(int i = 0; i < SurveyMakerWizard::pageNames.count(); i++) {
        if(i > 0) {
            title += " &emsp;/&emsp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == page) {
            title += "</span><span style=\"color: #" OPENWATERHEX "\">";
        }
    }
    title += "</span>";
    QString label = "  " + SurveyMakerWizard::pageNames[page];
    if(page != SurveyMakerWizard::Page::previewexport) {
        label += " " + tr("Questions");
    }

    layout = new QGridLayout(this);
    layout->setSpacing(0);

    pageTitle = new QLabel(title, this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(pageTitle, 0, 0, 1, -1);

    topLabel = new QLabel(label, this);
    topLabel->setStyleSheet(TOPLABELSTYLE);
    topLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    topLabel->setMinimumHeight(40);
    topLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    layout->addWidget(topLabel, 1, 0, 1, -1);

    questionWidget = new QWidget;
    questionLayout = new QVBoxLayout;
    questionLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    questionLayout->setSpacing(0);
    questionArea = new QScrollArea;
    questionArea->setWidget(questionWidget);
    questionArea->setStyleSheet("QScrollArea{border: none;}");
    questionArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    questionArea->setWidgetResizable(true);
    questionWidget->setLayout(questionLayout);

    if(page != SurveyMakerWizard::Page::previewexport) {
        layout->addWidget(questionArea, 2, 0, -1, 1);

        previewWidget = new QWidget;
        previewWidget->setStyleSheet("background-color: #ebebeb;");
        previewLayout = new QVBoxLayout;
        previewLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
        previewLayout->setSpacing(0);
        previewArea = new QScrollArea;
        previewArea->setWidget(previewWidget);
        previewArea->setStyleSheet("QScrollArea{background-color: #ebebeb; border: none;}");
        previewArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        previewArea->setWidgetResizable(true);
        previewWidget->setLayout(previewLayout);
        layout->addWidget(previewArea, 2, 1, -1, 1);

        layout->setColumnStretch(0,1);
        layout->setColumnStretch(1,1);

        if(page != SurveyMakerWizard::Page::multichoice) {
            for(int i = 0; i < numQuestions; i++) {
                questions << new SurveyMakerQuestionWithSwitch;
                questionPreviews << new QWidget;
                questionPreviewLayouts << new QVBoxLayout;
                questionPreviewTopLabels << new QLabel;
                questionPreviewBottomLabels << new QLabel;

                questionLayout->addSpacing(10);
                questionLayout->addWidget(questions.last());

                questionPreviews.last()->setLayout(questionPreviewLayouts.last());
                previewLayout->addWidget(questionPreviews.last());
                questionPreviews.last()->setAttribute(Qt::WA_TransparentForMouseEvents);
                questionPreviews.last()->setFocusPolicy(Qt::NoFocus);
                questionPreviewTopLabels.last()->setStyleSheet(LABELSTYLE);
                questionPreviewBottomLabels.last()->setStyleSheet(LABELSTYLE);
                connect(questions.last(), &SurveyMakerQuestionWithSwitch::valueChanged, questionPreviews.last(), &QWidget::setVisible);
            }
        }
        questionLayout->addStretch(1);
        previewLayout->addStretch(1);
    }
    else {
        layout->addWidget(questionArea, 2, 0, -1, -1);
        layout->setColumnStretch(0,1);
    }
    layout->addItem(new QSpacerItem(0,0), 2, 0);
    layout->setRowStretch(2, 1);
}


IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString title = "<span style=\"color: #" DEEPWATERHEX "\">";
    for(int i = 0, j = SurveyMakerWizard::pageNames.count(); i < j; i++) {
        if(i > 0) {
            title += " &emsp;/&emsp; ";
        }
        title += SurveyMakerWizard::pageNames[i];
        if(i == 0) {
            title += "</span><span style=\"color: #" OPENWATERHEX "\">";
        }
    }
    pageTitle = new QLabel(title, this);
    pageTitle->setStyleSheet(TITLESTYLE);
    pageTitle->setAlignment(Qt::AlignCenter);
    pageTitle->setScaledContents(true);
    pageTitle->setMinimumHeight(40);
    pageTitle->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    bannerLeft = new QLabel;
    bannerLeft->setStyleSheet("QLabel {background-color: #" DEEPWATERHEX ";}");
    QPixmap leftPixmap(":/icons_new/BannerLeft.png");
    bannerLeft->setPixmap(leftPixmap.scaledToHeight(120, Qt::SmoothTransformation));
    bannerLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    bannerLeft->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bannerRight = new QLabel;
    bannerRight->setStyleSheet("QLabel {background-color: #" DEEPWATERHEX ";}");
    QPixmap rightPixmap(":/icons_new/BannerRight.png");
    bannerRight->setPixmap(rightPixmap.scaledToHeight(120, Qt::SmoothTransformation));
    bannerRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bannerRight->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    banner = new QLabel;
    QString labelText = "<html><body>"
                        "<span style=\"font-family: 'Paytone One'; font-size: 24pt; color: white;\">"
                        "Make a survey with gruepr</span><br>"
                        "<span style=\"font-family: 'DM Sans'; font-size: 16pt; color: white;\">"
                        "Creating optimal grueps is easy! Get started with our five step survey-making flow below.</span>"
                        "</body></html>";
    banner->setText(labelText);
    banner->setAlignment(Qt::AlignCenter);
    banner->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    banner->setWordWrap(true);
    banner->setStyleSheet("QLabel {background-color: #" DEEPWATERHEX "; color: white;}");
    auto *bannerLayout = new QHBoxLayout;
    bannerLayout->setSpacing(0);
    bannerLayout->setContentsMargins(0, 0, 0, 0);
    bannerLayout->addWidget(bannerLeft);
    bannerLayout->addWidget(banner);
    bannerLayout->addWidget(bannerRight);

    topLabel = new QLabel(this);
    topLabel->setText("<span style=\"color: #" DEEPWATERHEX "; font-size: 12pt; font-family: DM Sans;\">" +
                      tr("Survey Name") + "</span>");
    topLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    surveyTitle = new QLineEdit(this);
    surveyTitle->setPlaceholderText(tr("Enter Text"));
    surveyTitle->setStyleSheet("color: #" DEEPWATERHEX "; font-size: 14pt; font-family: DM Sans; "
                               "border-style: outset; border-width: 2px; border-color: #" DEEPWATERHEX "; ");
    surveyTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    registerField("SurveyTitle", surveyTitle);

    bottomLabel = new QLabel(this);
    bottomLabel->setText("<span style=\"color: #" DEEPWATERHEX "; font-size: 10pt; font-family: DM Sans\">" +
                         tr("This will be the name of the survey you send to your students!") + "</span>");
    bottomLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    getStartedButton = new QPushButton("Get Started  \u2B62", this);
    getStartedButton->setStyleSheet(GETSTARTEDBUTTONSTYLE);
    getStartedButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    layout = new QGridLayout;
    layout->setSpacing(0);
    int row = 0;
    layout->addWidget(pageTitle, row++, 0, 1, -1);
    layout->addLayout(bannerLayout, row++, 0, 1, -1);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(topLabel, row++, 1, Qt::AlignLeft | Qt::AlignBottom);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(surveyTitle, row++, 1);
    layout->setRowMinimumHeight(row++, 10);
    layout->addWidget(bottomLabel, row++, 1, Qt::AlignLeft | Qt::AlignTop);
    layout->setRowMinimumHeight(row++, 20);
    layout->addWidget(getStartedButton, row++, 1);
    layout->setRowMinimumHeight(row, 0);
    layout->setRowStretch(row, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 3);
    layout->setColumnStretch(2, 1);
    setLayout(layout);
}

void IntroPage::initializePage()
{
    connect(getStartedButton, &QPushButton::clicked, wizard(), &QWizard::next);
}


DemographicsPage::DemographicsPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::demographics, parent)
{
    questions[firstname]->setLabel(tr("First name"));
    questionPreviewTopLabels[firstname]->setText(tr("First name"));
    questionPreviewLayouts[firstname]->addWidget(questionPreviewTopLabels[firstname]);
    fn = new QLineEdit(FIRSTNAMEQUESTION);
    fn->setCursorPosition(0);
    fn->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[firstname]->addWidget(fn);
    questionPreviewBottomLabels[firstname]->hide();
    questionPreviewLayouts[firstname]->addWidget(questionPreviewBottomLabels[firstname]);
    questionPreviews[firstname]->hide();
    registerField("FirstName", questions[firstname], "value", "valueChanged");

    questions[lastname]->setLabel(tr("Last name"));
    questionPreviewTopLabels[lastname]->setText(tr("Last name"));
    questionPreviewLayouts[lastname]->addWidget(questionPreviewTopLabels[lastname]);
    ln = new QLineEdit(LASTNAMEQUESTION);
    ln->setCursorPosition(0);
    ln->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[lastname]->addWidget(ln);
    questionPreviewBottomLabels[lastname]->hide();
    questionPreviewLayouts[lastname]->addWidget(questionPreviewBottomLabels[lastname]);
    questionPreviews[lastname]->hide();
    registerField("LastName", questions[lastname], "value", "valueChanged");

    questions[email]->setLabel(tr("Email"));
    questionPreviewTopLabels[email]->setText(tr("Email"));
    questionPreviewLayouts[email]->addWidget(questionPreviewTopLabels[email]);
    em = new QLineEdit(EMAILQUESTION);
    em->setCursorPosition(0);
    em->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[email]->addWidget(em);
    questionPreviewBottomLabels[email]->hide();
    questionPreviewLayouts[email]->addWidget(questionPreviewBottomLabels[email]);
    questionPreviews[email]->hide();
    registerField("Email", questions[email], "value", "valueChanged");

    questions[gender]->setLabel(tr("Gender"));
    auto *genderResponses = new QWidget;
    auto *genderResponsesLayout = new QHBoxLayout(genderResponses);
    genderResponsesLabel = new QLabel(tr("Ask as: "));
    genderResponsesLabel->setStyleSheet(LABELSTYLE);
    genderResponsesLabel->setEnabled(false);
    genderResponsesLayout->addWidget(genderResponsesLabel);
    genderResponsesComboBox = new QComboBox;
    genderResponsesComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    genderResponsesComboBox->installEventFilter(new MouseWheelBlocker(genderResponsesComboBox));
    genderResponsesComboBox->addItems({tr("Biological Sex"), tr("Adult Identity"), tr("Child Identity"), tr("Pronouns")});
    genderResponsesComboBox->setStyleSheet(COMBOBOXSTYLE);
    genderResponsesComboBox->setEnabled(false);
    genderResponsesComboBox->setCurrentIndex(3);
    genderResponsesLayout->addWidget(genderResponsesComboBox);
    genderResponsesLayout->addStretch(1);
    questions[gender]->addWidget(genderResponses, 1, 0, true);
    connect(genderResponsesComboBox, &QComboBox::currentIndexChanged, this, &DemographicsPage::update);
    questionPreviewTopLabels[gender]->setText(tr("Gender"));
    questionPreviewLayouts[gender]->addWidget(questionPreviewTopLabels[gender]);
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, this, &DemographicsPage::update);
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, genderResponsesLabel, &QLabel::setEnabled);
    connect(questions[gender], &SurveyMakerQuestionWithSwitch::valueChanged, genderResponsesComboBox, &QComboBox::setEnabled);
    ge = new QComboBox;
    ge->addItem(PRONOUNQUESTION);
    ge->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[gender]->addWidget(ge);
    questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    questionPreviewLayouts[gender]->addWidget(questionPreviewBottomLabels[gender]);
    questionPreviews[gender]->hide();
    registerField("Gender", questions[gender], "value", "valueChanged");
    registerField("genderOptions", genderResponsesComboBox);

    questions[urm]->setLabel(tr("Race / ethnicity"));
    questionPreviewTopLabels[urm]->setText(tr("Race / ethnicity"));
    questionPreviewLayouts[urm]->addWidget(questionPreviewTopLabels[urm]);
    re = new QLineEdit(URMQUESTION);
    re->setCursorPosition(0);
    re->setStyleSheet(LINEEDITSTYLE);
    questionPreviewLayouts[urm]->addWidget(re);
    questionPreviewBottomLabels[urm]->hide();
    questionPreviewLayouts[urm]->addWidget(questionPreviewBottomLabels[urm]);
    questionPreviews[urm]->hide();
    registerField("RaceEthnicity", questions[urm], "value", "valueChanged");

    update();
}

void DemographicsPage::initializePage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, DEEPWATER);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(STDBUTTONSTYLE);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton;
    wiz->setButtonLayout(buttonLayout);
}

void DemographicsPage::cleanupPage()
{
    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, Qt::white);
    wiz->setPalette(palette);
    wiz->button(QWizard::NextButton)->setStyleSheet(INVISBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(QString(NEXTBUTTONSTYLE).replace("border-color: white; ", "border-color: #" DEEPWATERHEX "; "));
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::CustomButton1 << QWizard::BackButton << QWizard::NextButton;
    wiz->setButtonLayout(buttonLayout);
}

void DemographicsPage::update()
{
    ge->clear();
    GenderType genderType = static_cast<GenderType>(genderResponsesComboBox->currentIndex());
    if(genderType == GenderType::biol)
    {
        ge->addItem(GENDERQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::adult)
    {
        ge->addItem(GENDERQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else if(genderType == GenderType::child)
    {
        ge->addItem(GENDERQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
    else //if(genderType == GenderType::pronoun)
    {
        ge->addItem(PRONOUNQUESTION);
        questionPreviewBottomLabels[gender]->setText(tr("Options: ") + QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE).join("  |  "));
    }
}


MultipleChoicePage::MultipleChoicePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::multichoice, parent)
{
    auto stretch = questionLayout->takeAt(0);   // will put this back at the end of the layout after adding everything
    sampleQuestionsFrame = new QFrame(this);
    sampleQuestionsFrame->setStyleSheet("background-color: " + (QColor::fromString("#"+QString(STARFISHHEX)).lighter(133).name()) + "; color: #" DEEPWATERHEX ";");
    sampleQuestionsIcon = new QLabel;
    sampleQuestionsIcon->setPixmap(QPixmap(":/icons_new/lightbulb.png").scaled(20,20,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    sampleQuestionsLabel = new QLabel(tr("Unsure of what to ask? Take a look at some example questions!"));
    sampleQuestionsLabel->setStyleSheet(LABELSTYLE);
    sampleQuestionsLabel->setWordWrap(true);
    sampleQuestionsLayout = new QHBoxLayout(sampleQuestionsFrame);
    sampleQuestionsButton = new QPushButton(tr("View Examples"));
    sampleQuestionsButton->setStyleSheet(EXAMPLEBUTTONSTYLE);
    sampleQuestionsDialog = new QDialog(this);
    connect(sampleQuestionsButton, &QPushButton::clicked, sampleQuestionsDialog, &QDialog::show);
    sampleQuestionsLayout->addWidget(sampleQuestionsIcon, 0, Qt::AlignLeft | Qt::AlignVCenter);
    sampleQuestionsLayout->addWidget(sampleQuestionsLabel, 1, Qt::AlignVCenter);
    sampleQuestionsLayout->addWidget(sampleQuestionsButton, 0, Qt::AlignRight | Qt::AlignVCenter);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(sampleQuestionsFrame);

    questionTexts.reserve(MAX_ATTRIBUTES);
    questionResponses.reserve(MAX_ATTRIBUTES);
    questionMultis.reserve(MAX_ATTRIBUTES);
    registerField("multiChoiceNumQuestions", this, "numQuestions", "numQuestionsChanged");
    registerField("multiChoiceQuestionTexts", this, "questionTexts", "questionTextsChanged");
    registerField("multiChoiceQuestionResponses", this, "questionResponses", "questionResponsesChanged");
    registerField("multiChoiceQuestionMultis", this, "questionMultis", "questionMultisChanged");

    for(int i = 0; i < (MAX_ATTRIBUTES - 1); i++) {
        //add the question
        spacers << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        questionLayout->addSpacerItem(spacers.last());
        multichoiceQuestions << new SurveyMakerMultichoiceQuestion(i + 1);
        questionLayout->addWidget(multichoiceQuestions.last());
        multichoiceQuestions.last()->hide();

        //add the preview
        questionPreviews << new QWidget;
        questionPreviews.last()->setAttribute(Qt::WA_TransparentForMouseEvents);
        questionPreviews.last()->setFocusPolicy(Qt::NoFocus);
        questionPreviewLayouts << new QVBoxLayout;
        questionPreviews.last()->setLayout(questionPreviewLayouts.last());
        QString fillInQuestion = tr("Question") + " " + QString::number(i + 1);
        questionPreviewTopLabels << new QLabel(fillInQuestion);
        questionPreviewTopLabels.last()->setStyleSheet(LABELSTYLE);
        questionPreviewLayouts.last()->addWidget(questionPreviewTopLabels.last());
        qu << new QComboBox;
        qu.last()->setStyleSheet(COMBOBOXSTYLE);
        qu.last()->setEditable(true);
        qu.last()->setCurrentText("[" + fillInQuestion + "]");
        questionPreviewLayouts.last()->addWidget(qu.last());
        questionPreviewBottomLabels << new QLabel(tr("Options") + ": ---");
        questionPreviewBottomLabels.last()->setStyleSheet(LABELSTYLE);
        questionPreviewLayouts.last()->addWidget(questionPreviewBottomLabels.last());
        previewLayout->insertWidget(i, questionPreviews.last());
        questionPreviews.last()->hide();

        //connect question to delete action and to updating the wizard fields and the preview
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::deleteRequested, this, [this, i]{deleteAQuestion(i);});
        questionTexts << "";
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::questionChanged, this, [this, i, fillInQuestion](const QString &newText)
                                                                                                     {questionTexts[i] = newText;
                                                                                                      qu[i]->setCurrentText(newText.isEmpty()? "[" + fillInQuestion + "]" : newText);});
        questionResponses << QStringList({""});
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::responsesChanged, this, [this, i](const QStringList &newResponses){questionResponses[i] = newResponses;});
        questionMultis << false;
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::multiChanged, this, [this, i](const bool newMulti){questionMultis[i] = newMulti;});
        connect(multichoiceQuestions.last(), &SurveyMakerMultichoiceQuestion::responsesAsStringChanged, questionPreviewBottomLabels.last(), &QLabel::setText);
    }

    addQuestionButtonFrame = new QFrame(this);
    addQuestionButtonFrame->setStyleSheet("background-color: #" BUBBLYHEX "; color: #" DEEPWATERHEX ";");
    addQuestionButtonLayout = new QHBoxLayout(addQuestionButtonFrame);
    addQuestionButton = new QPushButton;
    addQuestionButton->setStyleSheet(ADDBUTTONSTYLE);
    addQuestionButton->setText(tr("Create another question"));
    addQuestionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    connect(addQuestionButton, &QPushButton::clicked, this, &MultipleChoicePage::addQuestion);
    addQuestionButtonLayout->addWidget(addQuestionButton, 0, Qt::AlignVCenter);
    questionLayout->addSpacing(10);
    questionLayout->addWidget(addQuestionButtonFrame);
    questionLayout->addItem(stretch);

    addQuestion();
    addQuestion();
}

void MultipleChoicePage::initializePage()
{
}

void MultipleChoicePage::cleanupPage()
{
}

void MultipleChoicePage::setNumQuestions(const int newNumQuestions)
{
    while(numQuestions > 0) {
        deleteAQuestion(0);
    }
    while(numQuestions < newNumQuestions) {
        addQuestion();
    }
    emit numQuestionsChanged(numQuestions);
}

int MultipleChoicePage::getNumQuestions() const
{
    return numQuestions;
}

void MultipleChoicePage::setQuestionTexts(const QList<QString> &newQuestionTexts)
{
    int i = 0;
    for(const auto &newQuestionText : newQuestionTexts) {
        if(i >= numQuestions) {
            addQuestion();
        }
        multichoiceQuestions[i]->setQuestion(newQuestionText);
        i++;
    }
    questionTexts = newQuestionTexts;
    emit questionTextsChanged(questionTexts);
}

QList<QString> MultipleChoicePage::getQuestionTexts() const
{
    return questionTexts;
}

void MultipleChoicePage::setQuestionResponses(const QList<QList<QString>> &newQuestionResponses)
{
    int i = 0;
    for(const auto &newQuestionResponse : newQuestionResponses) {
        if(i >= numQuestions) {
            addQuestion();
        }
        multichoiceQuestions[i]->setResponses(newQuestionResponse);
        i++;
    }
    questionResponses = newQuestionResponses;
    emit questionResponsesChanged(questionResponses);
}

QList<QList<QString>> MultipleChoicePage::getQuestionResponses() const
{
    return questionResponses;
}

void MultipleChoicePage::setQuestionMultis(const QList<bool> &newQuestionMultis)
{
    int i = 0;
    for(const auto &newQuestionMulti : newQuestionMultis) {
        if(i >= numQuestions) {
            addQuestion();
        }
        multichoiceQuestions[i]->setMulti(newQuestionMulti);
        i++;
    }
    questionMultis = newQuestionMultis;
    emit questionMultisChanged(questionMultis);
}

QList<bool> MultipleChoicePage::getQuestionMultis() const
{
    return questionMultis;
}

void MultipleChoicePage::addQuestion()
{
    spacers[numQuestions]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    multichoiceQuestions[numQuestions]->show();
    questionPreviews[numQuestions]->show();

    numQuestions++;

    if(numQuestions == (MAX_ATTRIBUTES - 1)) {
        addQuestionButton->setEnabled(false);
        addQuestionButton->setToolTip(tr("Maximum number of questions reached."));
    }
}

void MultipleChoicePage::deleteAQuestion(int questionNum)
{
    //bump the data from every subsequent question up one
    for(int i = questionNum; i < (numQuestions - 1); i++) {
        multichoiceQuestions[i]->setQuestion(multichoiceQuestions[i+1]->getQuestion());
        multichoiceQuestions[i]->setResponses(multichoiceQuestions[i+1]->getResponses());
        multichoiceQuestions[i]->setMulti(multichoiceQuestions[i+1]->getMulti());
    }

    //clear the last question currently displayed, then hide it
    multichoiceQuestions[numQuestions - 1]->setQuestion("");
    multichoiceQuestions[numQuestions - 1]->setResponses(QStringList({""}));
    multichoiceQuestions[numQuestions - 1]->setMulti(false);

    spacers[numQuestions - 1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    multichoiceQuestions[numQuestions - 1]->hide();
    questionPreviews[numQuestions - 1]->hide();

    numQuestions--;

    if(numQuestions < (MAX_ATTRIBUTES - 1)) {
        addQuestionButton->setEnabled(true);
        addQuestionButton->setToolTip("");
    }
}


SchedulePage::SchedulePage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::schedule, parent)
{
    questions[timezone]->setLabel(tr("Timezone"));
    connect(questions[timezone], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[timezone]->setText(tr("Timezone"));
    questionPreviewLayouts[timezone]->addWidget(questionPreviewTopLabels[timezone]);
    tz = new QComboBox;
    tz->addItem(TIMEZONEQUESTION);
    tz->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[timezone]->addWidget(tz);
    questionPreviewBottomLabels[timezone]->setText(tr("Options: List of global timezones"));
    questionPreviewLayouts[timezone]->addWidget(questionPreviewBottomLabels[timezone]);
    questionPreviews[timezone]->hide();
    registerField("Timezone", questions[timezone], "value", "valueChanged");

    questions[schedule]->setLabel(tr("Schedule"));
    connect(questions[schedule], &SurveyMakerQuestionWithSwitch::valueChanged, this, &SchedulePage::update);
    questionPreviewTopLabels[schedule]->setText(generateScheduleQuestion(false, false, ""));
    questionPreviewLayouts[schedule]->addWidget(questionPreviewTopLabels[schedule]);
    sc = new QWidget;
    scLayout = new QGridLayout(sc);
    for(int hr = 0; hr < 24; hr++) {
        auto *rowLabel = new QLabel(SurveyMakerWizard::sunday.time().addSecs(hr * 3600).toString("h A"));
        rowLabel->setStyleSheet(LABELSTYLE);
        scLayout->addWidget(rowLabel, hr+1, 0);
    }
    for(int day = 0; day < 7; day++) {
        auto *colLabel = new QLabel(SurveyMakerWizard::sunday.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(LABELSTYLE);
        scLayout->addWidget(colLabel, 0, day+1);
    }
    for(int hr = 1; hr <= 24; hr++) {
        for(int day = 1; day <= 7; day++) {
            auto check = new QCheckBox;
            check->setChecked(true);
            check->setStyleSheet(CHECKBOXSTYLE);
            scLayout->addWidget(check, hr, day);
        }
    }
    scLayout->setSpacing(5);
    scLayout->setColumnStretch(7, 1);
    scLayout->setRowStretch(24, 1);
    questionPreviewLayouts[schedule]->addWidget(sc);
    questionPreviewBottomLabels[schedule]->hide();
    questionPreviewLayouts[schedule]->addWidget(questionPreviewBottomLabels[schedule]);
    questionPreviews[schedule]->hide();
    registerField("Schedule", questions[schedule], "value", "valueChanged");

    //subItems inside schedule question
    int row = 1;

    auto *busyOrFree = new QWidget;
    auto *busyOrFreeLayout = new QHBoxLayout(busyOrFree);
    busyOrFreeLabel = new QLabel(tr("Ask as: "));
    busyOrFreeLabel->setStyleSheet(LABELSTYLE);
    busyOrFreeLabel->setEnabled(false);
    busyOrFreeLayout->addWidget(busyOrFreeLabel);
    busyOrFreeComboBox = new QComboBox;
    busyOrFreeComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    busyOrFreeComboBox->installEventFilter(new MouseWheelBlocker(busyOrFreeComboBox));
    busyOrFreeComboBox->insertItem(busy, tr("Busy"));
    busyOrFreeComboBox->insertItem(free, tr("Free"));
    busyOrFreeComboBox->setStyleSheet(COMBOBOXSTYLE);
    busyOrFreeComboBox->setEnabled(false);
    busyOrFreeComboBox->setCurrentIndex(0);
    busyOrFreeLayout->addWidget(busyOrFreeComboBox);
    busyOrFreeLayout->addStretch(1);
    questions[schedule]->addWidget(busyOrFree, row++, 0, true);
    connect(busyOrFreeComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleBusyOrFree", busyOrFreeComboBox);

    baseTimezoneLabel = new QLabel(tr("Select timezone"));
    baseTimezoneLabel->setStyleSheet(LABELSTYLE);
    baseTimezoneLabel->hide();
    questions[schedule]->addWidget(baseTimezoneLabel, row++, 0, false);
    timeZoneNames = QString(TIMEZONENAMES).split(";");
    for(auto &timeZoneName : timeZoneNames)
    {
        timeZoneName.remove('"');
    }
    baseTimezoneComboBox = new ComboBoxWithElidedContents("Pacific: US and Canada, Tijuana [GMT-08:00]", this);
    baseTimezoneComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    baseTimezoneComboBox->installEventFilter(new MouseWheelBlocker(baseTimezoneComboBox));
    baseTimezoneComboBox->setStyleSheet(COMBOBOXSTYLE);
    baseTimezoneComboBox->setToolTip(tr("<html>Description of the timezone students should use to interpret the times in the grid.&nbsp;"
                                        "<b>Be aware how the meaning of the times in the grid changes depending on this setting.</b></html>"));
    baseTimezoneComboBox->addItem(tr("[student's home timezone]"));
    baseTimezoneComboBox->addItem(tr("Custom timezone:"));
    baseTimezoneComboBox->insertSeparator(2);
    for(int zone = 0; zone < timeZoneNames.size(); zone++)
    {
        const QString &zonename = timeZoneNames.at(zone);
        baseTimezoneComboBox->addItem(zonename);
        baseTimezoneComboBox->setItemData(3 + zone, zonename, Qt::ToolTipRole);
    }
    questions[schedule]->addWidget(baseTimezoneComboBox, row++, 0, true);
    baseTimezoneComboBox->hide();
    connect(baseTimezoneComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    customBaseTimezone = new QLineEdit;
    customBaseTimezone->setStyleSheet(LINEEDITSTYLE);
    customBaseTimezone->setPlaceholderText(tr("Custom timezone"));
    questions[schedule]->addWidget(customBaseTimezone, row++, 0, true);
    customBaseTimezone->hide();
    connect(customBaseTimezone, &QLineEdit::textChanged, this, &SchedulePage::update);
    registerField("baseTimezone", this, "baseTimezone", "baseTimezoneChanged");
    registerField("ScheduleQuestion", this, "scheduleQuestion", "scheduleQuestionChanged");

    timespanLabel = new QLabel(tr("Timespan:"));
    timespanLabel->setStyleSheet(LABELSTYLE);
    timespanLabel->setEnabled(false);
    questions[schedule]->addWidget(timespanLabel, row++, 0, false);
    daysComboBox = new QComboBox;
    daysComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    daysComboBox->installEventFilter(new MouseWheelBlocker(daysComboBox));
    daysComboBox->addItems({tr("All days"), tr("Weekdays"), tr("Weekends"), tr("Custom days/daynames")});
    daysComboBox->setStyleSheet(COMBOBOXSTYLE);
    daysComboBox->setEnabled(false);
    daysComboBox->setCurrentIndex(0);
    questions[schedule]->addWidget(daysComboBox, row++, 0, true, Qt::AlignLeft);
    connect(daysComboBox, &QComboBox::activated, this, &SchedulePage::daysComboBox_activated);

    //connect subwindow ui to slots
    dayNames.reserve(MAX_DAYS);
    dayCheckBoxes.reserve(MAX_DAYS);
    dayLineEdits.reserve(MAX_DAYS);
    for(int day = 0; day < MAX_DAYS; day++)
    {
        dayNames << SurveyMakerWizard::defaultDayNames.at(day);
        dayCheckBoxes << new QCheckBox;
        dayLineEdits <<  new QLineEdit;
        dayCheckBoxes.last()->setStyleSheet(CHECKBOXSTYLE);
        dayCheckBoxes.last()->setChecked(true);
        dayLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
        dayLineEdits.last()->setText(dayNames.at(day));
        dayLineEdits.last()->setPlaceholderText(tr("Day ") + QString::number(day + 1) + tr(" name"));
        connect(dayLineEdits.last(), &QLineEdit::textChanged, this, [this, day](const QString &text)
                                        {day_LineEdit_textChanged(text, dayLineEdits[day], dayNames[day]);});
        connect(dayLineEdits.last(), &QLineEdit::editingFinished, this, [this, day]
                                        {if(dayNames.at(day).isEmpty()){dayCheckBoxes[day]->setChecked(false);};});
        connect(dayCheckBoxes.last(), &QCheckBox::toggled, this, [this, day](bool checked)
                                        {day_CheckBox_toggled(checked, dayLineEdits[day], SurveyMakerWizard::defaultDayNames.at(day));});
    }
    daysWindow = new dayNamesDialog(dayCheckBoxes, dayLineEdits, this);
    registerField("scheduleDayNames", this, "dayNames", "dayNamesChanged");

    auto *fromTo = new QWidget;
    auto *fromToLayout = new QHBoxLayout(fromTo);
    fromLabel = new QLabel(tr("From"));
    fromLabel->setStyleSheet(LABELSTYLE);
    fromLabel->setEnabled(false);
    fromToLayout->addWidget(fromLabel, 0, Qt::AlignCenter);
    fromComboBox = new QComboBox;
    fromComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    fromComboBox->installEventFilter(new MouseWheelBlocker(fromComboBox));
    fromComboBox->setStyleSheet(COMBOBOXSTYLE);
    fromComboBox->setEnabled(false);
    fromToLayout->addWidget(fromComboBox, 0, Qt::AlignCenter);
    toLabel = new QLabel(tr("to"));
    toLabel->setStyleSheet(LABELSTYLE);
    toLabel->setEnabled(false);
    fromToLayout->addWidget(toLabel, 0, Qt::AlignCenter);
    toComboBox = new QComboBox;
    toComboBox->setFocusPolicy(Qt::StrongFocus);    // make scrollwheel scroll the question area, not the combobox value
    toComboBox->installEventFilter(new MouseWheelBlocker(toComboBox));
    toComboBox->setStyleSheet(COMBOBOXSTYLE);
    toComboBox->setEnabled(false);
    for(int hr = 0; hr < 24; hr++) {
        QString time = SurveyMakerWizard::sunday.time().addSecs(hr * 3600).toString("h A");
        fromComboBox->addItem(time);
        toComboBox->addItem(time);
    }
    fromComboBox->setCurrentIndex(STANDARDSCHEDSTARTTIME);
    toComboBox->setCurrentIndex(STANDARDSCHEDENDTIME);
    fromToLayout->addWidget(toComboBox, 0, Qt::AlignCenter);
    fromToLayout->addStretch(1);
    questions[schedule]->addWidget(fromTo, row++, 0, true);
    connect(fromComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleFrom", fromComboBox);
    connect(toComboBox, &QComboBox::currentIndexChanged, this, &SchedulePage::update);
    registerField("scheduleTo", toComboBox);

    update();
}

void SchedulePage::cleanupPage()
{
}

void SchedulePage::setDayNames(const QStringList &newDayNames)
{
    dayNames = newDayNames;
    while(dayNames.size() < MAX_DAYS) {
        dayNames << "";
    }
    for(int i = 0; i < MAX_DAYS; i++) {
        dayLineEdits[i]->setText(dayNames[i]);
        dayCheckBoxes[i]->setChecked(!dayNames[i].isEmpty());
    }
    emit dayNamesChanged(dayNames);
}

QStringList SchedulePage::getDayNames() const
{
    return dayNames;
}

void SchedulePage::setScheduleQuestion(const QString &newScheduleQuestion)
{
    scheduleQuestion = newScheduleQuestion;
    questionPreviewTopLabels[schedule]->setText(scheduleQuestion);
    emit scheduleQuestionChanged(scheduleQuestion);
}

QString SchedulePage::getScheduleQuestion() const
{
    return scheduleQuestion;
}

void SchedulePage::setBaseTimezone(const QString &newBaseTimezone)
{
    int index = baseTimezoneComboBox->findText(newBaseTimezone, Qt::MatchFixedString);
    if(index != -1) {
        baseTimezoneComboBox->setCurrentIndex(index);
    }
    else {
        baseTimezoneComboBox->setCurrentIndex(1);
        customBaseTimezone->show();
        customBaseTimezone->setEnabled(baseTimezoneComboBox->isEnabled());
        customBaseTimezone->setText(newBaseTimezone);
    }
    QString newScheduleQuestion = generateScheduleQuestion(busyOrFreeComboBox->currentIndex() == busy, questions[timezone]->getValue(), newBaseTimezone);
    setScheduleQuestion(newScheduleQuestion);
    baseTimezone = newBaseTimezone;
    emit baseTimezoneChanged(newBaseTimezone);
}

QString SchedulePage::getBaseTimezone() const
{
    return baseTimezone;
}

void SchedulePage::daysComboBox_activated(int index)
{
    daysComboBox->blockSignals(true);
    if(index == 0)
    {
        //All Days
        for(auto &dayCheckBox : dayCheckBoxes)
        {
            dayCheckBox->setChecked(true);
        }
    }
    else if(index == 1)
    {
        //Weekdays
        dayCheckBoxes[Sun]->setChecked(false);
        for(int day = Mon; day <= Fri; day++)
        {
            dayCheckBoxes[day]->setChecked(true);
        }
        dayCheckBoxes[Sat]->setChecked(false);
    }
    else if(index == 2)
    {
        //Weekends
        dayCheckBoxes[Sun]->setChecked(true);
        for(int day = Mon; day <= Fri; day++)
        {
            dayCheckBoxes[day]->setChecked(false);
        }
        dayCheckBoxes[Sat]->setChecked(true);
    }
    else
    {
        //Custom Days, open subwindow
        daysWindow->exec();
        checkDays();
    }
    daysComboBox->blockSignals(false);
    update();
}

void SchedulePage::day_CheckBox_toggled(bool checked, QLineEdit *dayLineEdit, const QString &dayname)
{
    dayLineEdit->setText(checked? dayname : "");
    dayLineEdit->setEnabled(checked);
    checkDays();
    update();
}

void SchedulePage::checkDays()
{
    bool weekends = dayCheckBoxes[Sun]->isChecked() && dayCheckBoxes[Sat]->isChecked();
    bool noWeekends = !(dayCheckBoxes[Sun]->isChecked() || dayCheckBoxes[Sat]->isChecked());
    bool weekdays = dayCheckBoxes[Mon]->isChecked() && dayCheckBoxes[Tue]->isChecked() &&
                    dayCheckBoxes[Wed]->isChecked() && dayCheckBoxes[Thu]->isChecked() && dayCheckBoxes[Fri]->isChecked();
    bool noWeekdays = !(dayCheckBoxes[Mon]->isChecked() || dayCheckBoxes[Tue]->isChecked() ||
                        dayCheckBoxes[Wed]->isChecked() || dayCheckBoxes[Thu]->isChecked() || dayCheckBoxes[Fri]->isChecked());
    if(weekends && weekdays)
    {
        daysComboBox->setCurrentIndex(0);
    }
    else if(weekdays && noWeekends)
    {
        daysComboBox->setCurrentIndex(1);
    }
    else if(weekends && noWeekdays)
    {
        daysComboBox->setCurrentIndex(2);
    }
    else
    {
        daysComboBox->setCurrentIndex(3);
    }
}

void SchedulePage::day_LineEdit_textChanged(const QString &text, QLineEdit *dayLineEdit, QString &dayname)
{
    //validate entry
    QString currText = text;
    int currPos = 0;
    if(SurveyMakerWizard::noInvalidPunctuation.validate(currText, currPos) != QValidator::Acceptable)
    {
        SurveyMakerWizard::invalidExpression(dayLineEdit, currText, this);
    }
    dayname = currText.trimmed();
    update();
}

void SchedulePage::update()
{
    //update the schedule grid
    QList<QWidget *> widgets = sc->findChildren<QWidget *>();
    for(auto &widget : widgets) {
        int row, col, rowSpan, colSpan;
        scLayout->getItemPosition(scLayout->indexOf(widget), &row, &col, &rowSpan, &colSpan);
        widget->setVisible(((row == 0) || ((row-1) >= fromComboBox->currentIndex() && (row-1) <= toComboBox->currentIndex())) &&
                           ((col == 0) || dayCheckBoxes[col-1]->isChecked()));
        if((row == 0) && (col > 0)) {
            auto *dayLabel = qobject_cast<QLabel *>(widget);
            if(dayLabel != nullptr) {
                dayLabel->setText((dayNames[col-1]).left(3));
            }
        }
    }

    if(fromComboBox->currentIndex() > toComboBox->currentIndex()) {
        fromComboBox->setStyleSheet(ERRORCOMBOBOXSTYLE);
        toComboBox->setStyleSheet(ERRORCOMBOBOXSTYLE);
    }
    else {
        fromComboBox->setStyleSheet(COMBOBOXSTYLE);
        toComboBox->setStyleSheet(COMBOBOXSTYLE);
    }

    bool scheduleOn = questions[schedule]->getValue();
    bool timezoneOn = questions[timezone]->getValue();

    baseTimezoneLabel->setVisible(timezoneOn);
    baseTimezoneLabel->setEnabled(scheduleOn);
    baseTimezoneComboBox->setVisible(timezoneOn);
    baseTimezoneComboBox->setEnabled(scheduleOn);
    customBaseTimezone->setVisible(baseTimezoneComboBox->currentIndex() == 1);
    customBaseTimezone->setEnabled(scheduleOn);
    switch(baseTimezoneComboBox->currentIndex()) {
    case 0:
        baseTimezone = SCHEDULEQUESTIONHOME;
        break;
    case 1:
        baseTimezone = customBaseTimezone->text();
        break;
    default:
        baseTimezone = baseTimezoneComboBox->currentText();
        break;
    }
    scheduleQuestion = generateScheduleQuestion(busyOrFreeComboBox->currentIndex() == busy, timezoneOn, baseTimezone);
    questionPreviewTopLabels[schedule]->setText(scheduleQuestion);

    busyOrFreeLabel->setEnabled(scheduleOn);
    busyOrFreeComboBox->setEnabled(scheduleOn);
    timespanLabel->setEnabled(scheduleOn);
    daysComboBox->setEnabled(scheduleOn);
    fromLabel->setEnabled(scheduleOn);
    fromComboBox->setEnabled(scheduleOn);
    toLabel->setEnabled(scheduleOn);
    toComboBox->setEnabled(scheduleOn);
}

QString SchedulePage::generateScheduleQuestion(bool scheduleAsBusy, bool timezoneOn, const QString &baseTimezone)
{
    QString questionText = SCHEDULEQUESTION1;
    questionText += (scheduleAsBusy? SCHEDULEQUESTION2BUSY : SCHEDULEQUESTION2FREE);
    questionText += SCHEDULEQUESTION3;
    if(timezoneOn) {
        questionText += SCHEDULEQUESTION4 + baseTimezone;
    }
    return questionText;
}


CourseInfoPage::CourseInfoPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::courseinfo, parent)
{
    questions[section]->setLabel(tr("Section"));
    connect(questions[section], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    addSectionButton = new QPushButton;
    addSectionButton->setStyleSheet(ADDBUTTONSTYLE);
    addSectionButton->setText(tr("Add section"));
    addSectionButton->setIcon(QIcon(":/icons_new/addButton.png"));
    addSectionButton->setEnabled(false);
    questions[section]->addWidget(addSectionButton, 1, 0, false, Qt::AlignLeft);
    connect(addSectionButton, &QPushButton::clicked, this, &CourseInfoPage::addASection);
    sectionLineEdits.reserve(10);
    deleteSectionButtons.reserve(10);
    sectionNames.reserve(10);

    questionPreviewTopLabels[section]->setText(tr("Section"));
    questionPreviewLayouts[section]->addWidget(questionPreviewTopLabels[section]);
    sc = new QComboBox;
    sc->addItem(SECTIONQUESTION);
    sc->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[section]->addWidget(sc);
    questionPreviewLayouts[section]->addWidget(questionPreviewBottomLabels[section]);
    questionPreviews[section]->hide();
    registerField("Section", questions[section], "value", "valueChanged");
    registerField("SectionNames", this, "sectionNames", "sectionNamesChanged");

    questions[wantToWorkWith]->setLabel(tr("Classmates I want to work with"));
    connect(questions[wantToWorkWith], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    questionPreviewTopLabels[wantToWorkWith]->setText(tr("Classmates"));
    questionPreviewLayouts[wantToWorkWith]->addWidget(questionPreviewTopLabels[wantToWorkWith]);
    ww = new QLineEdit;
    ww->setCursorPosition(0);
    ww->setStyleSheet(LINEEDITSTYLE);
    wwc = new QComboBox;
    wwc->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[wantToWorkWith]->addWidget(ww);
    questionPreviewLayouts[wantToWorkWith]->addWidget(wwc);
    questionPreviewBottomLabels[wantToWorkWith]->setText(tr(""));
    questionPreviewLayouts[wantToWorkWith]->addWidget(questionPreviewBottomLabels[wantToWorkWith]);
    wwc->hide();
    questionPreviews[wantToWorkWith]->hide();
    questionPreviewBottomLabels[wantToWorkWith]->hide();
    registerField("PrefTeammate", questions[wantToWorkWith], "value", "valueChanged");

    questionLayout->removeItem(questionLayout->itemAt(4));

    questions[wantToAvoid]->setLabel(tr("Classmates I want to avoid"));
    connect(questions[wantToAvoid], &SurveyMakerQuestionWithSwitch::valueChanged, this, &CourseInfoPage::update);
    numPrefTeammatesExplainer = new QLabel(tr("Number of classmates a student can indicate:"));
    numPrefTeammatesExplainer->setWordWrap(true);
    numPrefTeammatesExplainer->setStyleSheet(LABELSTYLE);
    numPrefTeammatesSpinBox = new QSpinBox;
    numPrefTeammatesSpinBox->setStyleSheet(SPINBOXSTYLE);
    numPrefTeammatesSpinBox->setValue(1);
    numPrefTeammatesSpinBox->setMinimum(1);
    numPrefTeammatesSpinBox->setMaximum(10);
    registerField("numPrefTeammates", numPrefTeammatesSpinBox);
    connect(numPrefTeammatesSpinBox, &QSpinBox::valueChanged, this, &CourseInfoPage::update);
    questions[wantToAvoid]->addWidget(numPrefTeammatesExplainer, 1, 0, true);
    questions[wantToAvoid]->addWidget(numPrefTeammatesSpinBox, 2, 0, false, Qt::AlignLeft);
    questionPreviewTopLabels[wantToAvoid]->setText(tr("Classmates"));
    questionPreviewLayouts[wantToAvoid]->addWidget(questionPreviewTopLabels[wantToAvoid]);
    wa = new QLineEdit;
    wa->setCursorPosition(0);
    wa->setStyleSheet(LINEEDITSTYLE);
    wac = new QComboBox;
    wac->setStyleSheet(COMBOBOXSTYLE);
    questionPreviewLayouts[wantToAvoid]->addWidget(wa);
    questionPreviewLayouts[wantToAvoid]->addWidget(wac);
    questionPreviewBottomLabels[wantToAvoid]->setText(tr(""));
    questionPreviewLayouts[wantToAvoid]->addWidget(questionPreviewBottomLabels[wantToAvoid]);
    wac->hide();
    questionPreviewBottomLabels[wantToWorkWith]->hide();
    questionPreviews[wantToAvoid]->hide();
    registerField("PrefNonTeammate", questions[wantToAvoid], "value", "valueChanged");

    selectFromRosterLabel = new LabelThatForwardsMouseClicks;
    selectFromRosterLabel->setText("<span style=\"color: #" DEEPWATERHEX "; font-family:'DM Sans'; font-size:12pt\">" + tr("Select from a class roster") + "</span>");
    selectFromRosterSwitch = new SwitchButton(false);
    connect(selectFromRosterSwitch, &SwitchButton::valueChanged, this, &CourseInfoPage::update);
    connect(selectFromRosterLabel, &LabelThatForwardsMouseClicks::mousePressed, selectFromRosterSwitch, &SwitchButton::mousePressEvent);
    questions[wantToAvoid]->addWidget(selectFromRosterLabel, 3, 0, false);
    questions[wantToAvoid]->addWidget(selectFromRosterSwitch, 3, 1, false, Qt::AlignRight);
    uploadExplainer = new QLabel(tr("You can upload a class roster so that students select names rather than typing as a free response question"));
    uploadExplainer->setWordWrap(true);
    uploadExplainer->setStyleSheet(LABELSTYLE);
    uploadButton = new QPushButton;
    uploadButton->setStyleSheet(ADDBUTTONSTYLE);
    uploadButton->setText(tr("Upload class roster"));
    uploadButton->setIcon(QIcon(":/icons_new/addButton.png"));
    uploadButton->setEnabled(false);
    connect(uploadButton, &QPushButton::clicked, this, &CourseInfoPage::uploadRoster);
    questions[wantToAvoid]->addWidget(uploadExplainer, 4, 0, true);
    questions[wantToAvoid]->addWidget(uploadButton, 5, 0, false, Qt::AlignLeft);

    registerField("StudentNames", this, "studentNames", "studentNamesChanged");

    addASection();
    addASection();
}

void CourseInfoPage::initializePage()
{
}

void CourseInfoPage::cleanupPage()
{
}

void CourseInfoPage::setSectionNames(const QStringList &newSectionNames)
{
    for(int i = 0; i < sectionLineEdits.size(); i++) {
        deleteASection(i);
    }
    for(const auto &newSectionName : newSectionNames) {
        addASection();
        sectionLineEdits.last()->setText(newSectionName);
    }
    emit sectionNamesChanged(sectionNames);
}

QStringList CourseInfoPage::getSectionNames() const
{
    return sectionNames;
}

void CourseInfoPage::setStudentNames(const QStringList &newStudentNames)
{
    studentNames = newStudentNames;
    emit studentNamesChanged(studentNames);
}

QStringList CourseInfoPage::getStudentNames() const
{
    return studentNames;
}

void CourseInfoPage::update()
{
    int numVisibleSections = 0;
    sectionNames.clear();
    for(auto &sectionLineEdit : sectionLineEdits) {
        if(!(sectionLineEdit->text().isEmpty())) {
            sectionNames.append(sectionLineEdit->text());
        }
        if(sectionLineEdit->isVisible()) {
            numVisibleSections++;
        }
        sectionLineEdit->setEnabled(questions[section]->getValue());
    }
    for(auto &deleteSectionButton : deleteSectionButtons) {
        deleteSectionButton->setEnabled(questions[section]->getValue() && (numVisibleSections > 2));
    }
    addSectionButton->setEnabled(questions[section]->getValue());
    questionPreviewBottomLabels[section]->setText(tr("Options: ") + sectionNames.join("  |  "));

    bool oneOfPrefQuestions = (questions[wantToWorkWith]->getValue() || questions[wantToAvoid]->getValue());
    selectFromRosterLabel->setEnabled(oneOfPrefQuestions);
    selectFromRosterLabel->setText(selectFromRosterLabel->text().replace(DEEPWATERHEX, (oneOfPrefQuestions? DEEPWATERHEX : "bebebe")));
    selectFromRosterSwitch->setEnabled(oneOfPrefQuestions);
    numPrefTeammatesExplainer->setEnabled(oneOfPrefQuestions);
    numPrefTeammatesSpinBox->setEnabled(oneOfPrefQuestions);
    uploadExplainer->setEnabled(oneOfPrefQuestions);
    uploadButton->setEnabled(selectFromRosterSwitch->isEnabled() && selectFromRosterSwitch->value());

    questionPreviewTopLabels[wantToAvoid]->setHidden(questions[wantToWorkWith]->getValue() && questions[wantToAvoid]->getValue());  //don't need two labels if both questions on
    if(!selectFromRosterSwitch->value() || studentNames.isEmpty()) {
        ww->show();
        wa->show();
        wwc->hide();
        wac->hide();
        questionPreviewBottomLabels[wantToWorkWith]->hide();
        questionPreviewBottomLabels[wantToAvoid]->hide();
    }
    else {
        ww->hide();
        wa->hide();
        wwc->show();
        wac->show();
        questionPreviewBottomLabels[wantToWorkWith]->show();
        questionPreviewBottomLabels[wantToAvoid]->show();
        questionPreviewBottomLabels[wantToWorkWith]->setHidden(questions[wantToWorkWith]->getValue() && questions[wantToAvoid]->getValue());
        questionPreviewBottomLabels[wantToWorkWith]->setText(tr("Options: ") + studentNames.join("  |  "));
        questionPreviewBottomLabels[wantToAvoid]->setText(tr("Options: ") + studentNames.join("  |  "));
    }

    numPrefTeammates = numPrefTeammatesSpinBox->value();
    ww->setText(generateTeammateQuestion(true, true, numPrefTeammates));
    ww->setCursorPosition(0);
    wwc->clear();
    wwc->addItem(generateTeammateQuestion(true, false, numPrefTeammates));
    wa->setText(generateTeammateQuestion(false, true, numPrefTeammates));
    wa->setCursorPosition(0);
    wac->clear();
    wac->addItem(generateTeammateQuestion(false, false, numPrefTeammates));
}

QString CourseInfoPage::generateTeammateQuestion(bool wantToWorkWith, bool typingNames, int numClassmates)
{
    QString question;
    question += typingNames ? PREFTEAMMATEQUESTION1TYPE : PREFTEAMMATEQUESTION1SELECT;

    question += numClassmates == 1 ? PREFTEAMMATEQUESTION2AMATE : PREFTEAMMATEQUESTION2MULTIA + QString::number(numClassmates) + PREFTEAMMATEQUESTION2MULTIB;

    question += wantToWorkWith ? PREFTEAMMATEQUESTION3YES : PREFTEAMMATEQUESTION3NO;

    if(typingNames) {
        question += numClassmates == 1 ? PREFTEAMMATEQUESTION4TYPEONE : PREFTEAMMATEQUESTION4TYPEMULTI;
    }

    return question;
}


void CourseInfoPage::addASection()
{
    sectionLineEdits.append(new QLineEdit);
    deleteSectionButtons.append(new QPushButton);
    sectionLineEdits.last()->setStyleSheet(LINEEDITSTYLE);
    deleteSectionButtons.last()->setStyleSheet(DELBUTTONSTYLE);
    sectionLineEdits.last()->setPlaceholderText(tr("Section name"));
    deleteSectionButtons.last()->setText(tr("Delete"));
    deleteSectionButtons.last()->setIcon(QIcon(":/icons_new/trashButton.png"));
    questions[section]->moveWidget(addSectionButton, numSectionsEntered + 2, 0, false, Qt::AlignLeft);
    questions[section]->addWidget(sectionLineEdits.last(), numSectionsEntered + 1, 0, false);
    sectionLineEdits.last()->show();
    questions[section]->addWidget(deleteSectionButtons.last(), numSectionsEntered + 1, 1, false);
    deleteSectionButtons.last()->show();
    connect(sectionLineEdits.last(), &QLineEdit::textChanged, this, &CourseInfoPage::update);
    connect(deleteSectionButtons.last(), &QPushButton::clicked, this, [this, numSectionsEntered = numSectionsEntered]{deleteASection(numSectionsEntered);});
    numSectionsEntered++;
    sectionLineEdits.last()->setFocus();

    update();
}

void CourseInfoPage::deleteASection(int sectionNum)
{
    sectionLineEdits[sectionNum]->hide();
    sectionLineEdits[sectionNum]->setText("");
    deleteSectionButtons[sectionNum]->hide();

    update();
}

void CourseInfoPage::uploadRoster()
{
    ////////////////////////get csv file, parse into names
    studentNames = {"Joshua Hertz", "Jasmine Lellock", "Cora Hertz", "Charlie Hertz"};

    setStudentNames(studentNames);

    update();
}


PreviewAndExportPage::PreviewAndExportPage(QWidget *parent)
    : SurveyMakerPage(SurveyMakerWizard::Page::previewexport, parent)
{
    for(int sectionNum = 0; sectionNum < 5; sectionNum++) {
        preSectionSpacer << new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        questionLayout->addItem(preSectionSpacer.last());
        section << new SurveyMakerPreviewSection(sectionNum, SurveyMakerWizard::pageNames[sectionNum], SurveyMakerWizard::numOfQuestionsInPage[sectionNum], this);
        questionLayout->addWidget(section.last());
        connect(section.last(), &SurveyMakerPreviewSection::editRequested, this, [this](int pageNum){while(pageNum < 5){wizard()->back(); pageNum++;}});
    }

    auto *saveExportFrame = new QFrame;
    saveExportFrame->setStyleSheet("background-color: #" DEEPWATERHEX "; color: white; font-family:'DM Sans'; font-size:12pt;");
    auto *saveExportlayout = new QVBoxLayout(saveExportFrame);
    auto *saveExporttitle = new QLabel("<span style=\"color: white; font-family:'DM Sans'; font-size:14pt;\">" + tr("Export Survey As:") + "</span>");
    auto *destination = new QGroupBox("");
    destination->setStyleSheet("border-style:none;");
    auto *radio1 = new QRadioButton(tr("Google Form"));
    auto *radio2 = new QRadioButton(tr("Canvas Survey"));
    auto *radio3 = new QRadioButton(tr("Text Files"));
    radio1->setChecked(true);
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(radio1);
    vbox->addWidget(radio2);
    vbox->addWidget(radio3);
    destination->setLayout(vbox);
    auto *exportButton = new QPushButton;
    exportButton->setStyleSheet(NEXTBUTTONSTYLE);
    exportButton->setText(tr("Export Survey"));
    connect(exportButton, &QPushButton::clicked, this, &PreviewAndExportPage::exportSurvey);
    auto *saveButton = new QPushButton;
    saveButton->setStyleSheet(NEXTBUTTONSTYLE);
    saveButton->setText(tr("Save Survey"));
    connect(saveButton, &QPushButton::clicked, this, &PreviewAndExportPage::saveSurvey);
    saveExportlayout->addWidget(saveExporttitle);
    saveExportlayout->addWidget(destination);
    saveExportlayout->addWidget(exportButton);
    saveExportlayout->addWidget(saveButton);
    preSectionSpacer << new QSpacerItem(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    questionLayout->addItem(preSectionSpacer.last());
    questionLayout->addWidget(saveExportFrame);

    questionLayout->addStretch(1);

    section[SurveyMakerWizard::demographics]->questionLabel[0]->setText(tr("First Name"));
    section[SurveyMakerWizard::demographics]->questionLineEdit[0]->setText(FIRSTNAMEQUESTION);
    section[SurveyMakerWizard::demographics]->questionLabel[1]->setText(tr("Last Name"));
    section[SurveyMakerWizard::demographics]->questionLineEdit[1]->setText(LASTNAMEQUESTION);
    section[SurveyMakerWizard::demographics]->questionLabel[2]->setText(tr("Email"));
    section[SurveyMakerWizard::demographics]->questionLineEdit[2]->setText(EMAILQUESTION);
    section[SurveyMakerWizard::demographics]->questionLabel[3]->setText(tr("Gender"));
    section[SurveyMakerWizard::demographics]->questionLabel[4]->setText(tr("Race / ethnicity"));
    section[SurveyMakerWizard::demographics]->questionLineEdit[4]->setText(URMQUESTION);

    section[SurveyMakerWizard::schedule]->questionLabel[0]->setText(tr("Timezone"));
    section[SurveyMakerWizard::schedule]->questionComboBox[0]->addItem(TIMEZONEQUESTION);
    section[SurveyMakerWizard::schedule]->questionComboBox[0]->insertSeparator(1);
    auto timeZoneNames = QString(TIMEZONENAMES).split(";");
    section[SurveyMakerWizard::schedule]->questionComboBox[0]->addItems(timeZoneNames);
    section[SurveyMakerWizard::schedule]->questionLabel[1]->setText(tr("Schedule"));
    schedGrid = new QWidget;
    schedGridLayout = new QGridLayout(schedGrid);
    for(int hr = 0; hr < 24; hr++) {
        auto *rowLabel = new QLabel(SurveyMakerWizard::sunday.time().addSecs(hr * 3600).toString("h A"));
        rowLabel->setStyleSheet(LABELSTYLE);
        schedGridLayout->addWidget(rowLabel, hr+1, 0);
    }
    for(int day = 0; day < 7; day++) {
        auto *colLabel = new QLabel(SurveyMakerWizard::sunday.addDays(day).toString("ddd"));
        colLabel->setStyleSheet(LABELSTYLE);
        schedGridLayout->addWidget(colLabel, 0, day+1);
        schedGridLayout->setColumnStretch(day, 1);
    }
    for(int hr = 1; hr <= 24; hr++) {
        for(int day = 1; day <= 7; day++) {
            auto check = new QCheckBox;
            check->setChecked(true);
            check->setStyleSheet(CHECKBOXSTYLE);
            schedGridLayout->addWidget(check, hr, day);
        }
    }
    schedGridLayout->setSpacing(5);
    schedGridLayout->setRowStretch(24, 1);
    section[SurveyMakerWizard::schedule]->addWidget(schedGrid);

    section[SurveyMakerWizard::courseinfo]->questionLabel[0]->setText(tr("Section"));
}

void PreviewAndExportPage::initializePage()
{
    survey = new Survey;

    auto *wiz = wizard();
    auto palette = wiz->palette();
    palette.setColor(QPalette::Window, DEEPWATER);
    wiz->setPalette(palette);
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton;
    wiz->setButtonLayout(buttonLayout);
    wiz->button(QWizard::NextButton)->setStyleSheet(NEXTBUTTONSTYLE);
    wiz->button(QWizard::CancelButton)->setStyleSheet(STDBUTTONSTYLE);

    //Survey title
    const QString title = field("SurveyTitle").toString().trimmed();
    preSectionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
    section[SurveyMakerWizard::introtitle]->setTitle(title);
    survey->title = title;

    //Demographics
    const bool firstname = field("FirstName").toBool();
    const bool lastname = field("LastName").toBool();
    const bool email = field("Email").toBool();
    const bool gender = field("Gender").toBool();
    const bool urm = field("RaceEthnicity").toBool();

    if(firstname) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[0]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[0]->show();
        survey->questions << Question(FIRSTNAMEQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[0]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[0]->hide();
    }

    if(lastname) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[1]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[1]->show();
        survey->questions << Question(LASTNAMEQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[1]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[1]->hide();
    }

    if(email) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[2]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[2]->show();
        survey->questions << Question(EMAILQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[2]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[2]->hide();
    }

    if(gender) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[3]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[3]->show();
        section[SurveyMakerWizard::demographics]->questionComboBox[3]->show();
        QString text;
        QStringList options;
        switch(static_cast<GenderType>(field("genderOptions").toInt())) {
        case GenderType::biol:
            text = GENDERQUESTION;
            options = QString(BIOLGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::adult:
            text = GENDERQUESTION;
            options = QString(ADULTGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::child:
            text = GENDERQUESTION;
            options = QString(CHILDGENDERS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        case GenderType::pronoun:
            text = PRONOUNQUESTION;
            options = QString(PRONOUNS).split('/').replaceInStrings(UNKNOWNVALUE, PREFERNOTRESPONSE);
            break;
        }
        section[SurveyMakerWizard::demographics]->questionComboBox[3]->clear();
        section[SurveyMakerWizard::demographics]->questionComboBox[3]->addItem(text);
        section[SurveyMakerWizard::demographics]->questionComboBox[3]->insertSeparator(1);
        section[SurveyMakerWizard::demographics]->questionComboBox[3]->addItems(options);
        survey->questions << Question(text, Question::QuestionType::dropdown, options);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[3]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[3]->hide();
        section[SurveyMakerWizard::demographics]->questionComboBox[3]->hide();
    }

    if(urm) {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[4]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[4]->show();
        section[SurveyMakerWizard::demographics]->questionLineEdit[4]->show();
        survey->questions << Question(URMQUESTION, Question::QuestionType::shorttext);
    }
    else {
        section[SurveyMakerWizard::demographics]->preQuestionSpacer[4]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->questionLabel[4]->hide();
        section[SurveyMakerWizard::demographics]->questionLineEdit[4]->hide();
    }

    if(firstname || lastname || email || gender || urm) {
        preSectionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->show();
    }
    else {
        preSectionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::demographics]->hide();
    }

    //Multiple Choice
    const int multiChoiceNumQuestions = field("multiChoiceNumQuestions").toInt();
    const QStringList multiQuestionTexts = field("multiChoiceQuestionTexts").toStringList();
    const auto multiQuestionResponses = field("multiChoiceQuestionResponses").toList();
    const auto multiQuestionMultis = field("multiChoiceQuestionMultis").toList();

    int actualNumMultiQuestions = 0;
    for(int questionNum = 0; questionNum < multiChoiceNumQuestions; questionNum++) {
        if(!multiQuestionTexts[questionNum].isEmpty()) {
            actualNumMultiQuestions++;
            section[SurveyMakerWizard::multichoice]->preQuestionSpacer[questionNum]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
            section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->setText(multiQuestionTexts[questionNum]);
            section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->show();
            auto responses = multiQuestionResponses[questionNum].toStringList();
            if((multiQuestionMultis[questionNum]).toBool()) {
                QLayoutItem *child;
                while((child = section[SurveyMakerWizard::multichoice]->questionGroupLayout[questionNum]->takeAt(0)) != nullptr) {
                    delete child->widget(); // delete the widget
                    delete child;   // delete the layout item
                }
                for(const auto &response : responses) {
                    auto *option = new QCheckBox(response);
                    option->setStyleSheet(CHECKBOXSTYLE);
                    section[SurveyMakerWizard::multichoice]->questionGroupLayout[questionNum]->addWidget(option);
                }
                section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->show();
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->hide();
                survey->questions << Question(multiQuestionTexts[questionNum], Question::QuestionType::checkbox, responses);
            }
            else {
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->clear();
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->addItems(responses);
                section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->show();
                section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->hide();
                survey->questions << Question(multiQuestionTexts[questionNum], Question::QuestionType::dropdown, responses);
            }
        }
        else {
            section[SurveyMakerWizard::multichoice]->preQuestionSpacer[questionNum]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
            section[SurveyMakerWizard::multichoice]->questionLabel[questionNum]->hide();
            section[SurveyMakerWizard::multichoice]->questionComboBox[questionNum]->hide();
            section[SurveyMakerWizard::multichoice]->questionGroupBox[questionNum]->hide();
        }
    }
    for(int i = actualNumMultiQuestions; i < MAX_ATTRIBUTES; i++) {
        section[SurveyMakerWizard::multichoice]->preQuestionSpacer[i]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::multichoice]->questionLabel[i]->hide();
        section[SurveyMakerWizard::multichoice]->questionComboBox[i]->hide();
        section[SurveyMakerWizard::multichoice]->questionGroupBox[i]->hide();
    }

    if(actualNumMultiQuestions > 0) {
        preSectionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::multichoice]->show();
    }
    else {
        preSectionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::multichoice]->hide();
    }

    //Schedule
    const bool timezone = field("Timezone").toBool();
    const bool schedule = field("Schedule").toBool();
    //const bool scheduleAsBusy = field("scheduleBusyOrFree").toBool();
    const QString scheduleQuestion = field("ScheduleQuestion").toString();
    const QStringList scheduleDays = field("scheduleDayNames").toStringList();
    const int scheduleFrom = field("scheduleFrom").toInt();
    const int scheduleTo = field("scheduleTo").toInt();

    if(timezone) {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[0]->show();
        section[SurveyMakerWizard::schedule]->questionComboBox[0]->show();
        survey->questions << Question(TIMEZONEQUESTION, Question::QuestionType::dropdown, QString(TIMEZONENAMES).split(";"));
    }
    else {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[0]->hide();
        section[SurveyMakerWizard::schedule]->questionComboBox[0]->hide();
    }

    if(schedule) {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[1]->setText(scheduleQuestion);
        section[SurveyMakerWizard::schedule]->questionLabel[1]->show();
        survey->questions << Question(scheduleQuestion, Question::QuestionType::schedule);
        for(int day = 0; day < MAX_DAYS; day++)
        {
            if(!scheduleDays.at(day).isEmpty())
            {
                survey->schedDayNames << scheduleDays.at(day);
            }
        }
        survey->schedStartTime = scheduleFrom;
        survey->schedEndTime = scheduleTo;
        QList<QWidget *> widgets = schedGrid->findChildren<QWidget *>();
        for(auto &widget : widgets) {
            int row, col, rowSpan, colSpan;
            schedGridLayout->getItemPosition(schedGridLayout->indexOf(widget), &row, &col, &rowSpan, &colSpan);
            widget->setVisible(((row == 0) || ((row-1) >= scheduleFrom && (row-1) <= scheduleTo)) &&
                               ((col == 0) || (!scheduleDays[col-1].isEmpty())));
            if((row == 0) && (col > 0)) {
                auto *dayLabel = qobject_cast<QLabel *>(widget);
                if(dayLabel != nullptr) {
                    dayLabel->setText(scheduleDays[col-1]);
                }
            }
        }
    }
    else {
        section[SurveyMakerWizard::schedule]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->questionLabel[1]->hide();
    }

    if(timezone || schedule) {
        preSectionSpacer[3]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->show();
    }
    else {
        preSectionSpacer[3]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::schedule]->hide();
    }

    //Course Info
    const bool courseSections = field("Section").toBool();
    const QStringList courseSectionsNames = field("SectionNames").toStringList();
    const bool prefTeammate = field("PrefTeammate").toBool();
    const bool prefNonTeammate = field("PrefNonTeammate").toBool();
    const int numPrefTeammates = field("numPrefTeammates").toInt();
    const QStringList studentNames = field("StudentNames").toStringList();

    if(courseSections) {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[0]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[0]->show();
        section[SurveyMakerWizard::courseinfo]->questionComboBox[0]->show();
        section[SurveyMakerWizard::courseinfo]->questionComboBox[0]->clear();
        section[SurveyMakerWizard::courseinfo]->questionComboBox[0]->addItem(SECTIONQUESTION);
        section[SurveyMakerWizard::courseinfo]->questionComboBox[0]->insertSeparator(1);
        section[SurveyMakerWizard::courseinfo]->questionComboBox[0]->addItems(courseSectionsNames);
        survey->questions << Question(SECTIONQUESTION, Question::QuestionType::radiobutton, courseSectionsNames);
    }
    else {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[0]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[0]->hide();
        section[SurveyMakerWizard::courseinfo]->questionComboBox[0]->hide();
    }

    if(prefTeammate) {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[1]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[1]->show();
        QString questionText = CourseInfoPage::generateTeammateQuestion(true, studentNames.isEmpty(), numPrefTeammates);
        if(studentNames.isEmpty()) {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->show();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->hide();
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->setText(questionText);
            survey->questions << Question(questionText, (numPrefTeammates == 1? Question::QuestionType::shorttext : Question::QuestionType::longtext));
        }
        else {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->hide();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->show();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->clear();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->addItem(questionText);
            section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->insertSeparator(1);
            section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->addItems(studentNames);
            survey->questions << Question(questionText, Question::QuestionType::dropdown, studentNames);
        }
    }
    else {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[1]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[1]->hide();
        section[SurveyMakerWizard::courseinfo]->questionLineEdit[1]->hide();
        section[SurveyMakerWizard::courseinfo]->questionComboBox[1]->hide();
    }

    if(prefNonTeammate) {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[2]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[2]->show();
        QString questionText = CourseInfoPage::generateTeammateQuestion(false, studentNames.isEmpty(), numPrefTeammates);
        if(studentNames.isEmpty()) {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->show();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->hide();
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->setText(questionText);
            survey->questions << Question(questionText, (numPrefTeammates == 1? Question::QuestionType::shorttext : Question::QuestionType::longtext));
        }
        else {
            section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->hide();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->show();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->clear();
            section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->addItem(questionText);
            section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->insertSeparator(1);
            section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->addItems(studentNames);
            survey->questions << Question(questionText, Question::QuestionType::dropdown, studentNames);
        }
    }
    else {
        section[SurveyMakerWizard::courseinfo]->preQuestionSpacer[2]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->questionLabel[2]->hide();
        section[SurveyMakerWizard::courseinfo]->questionLineEdit[2]->hide();
        section[SurveyMakerWizard::courseinfo]->questionComboBox[2]->hide();
    }

    if(courseSections || prefTeammate || prefNonTeammate) {
        preSectionSpacer[4]->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->show();
    }
    else {
        preSectionSpacer[4]->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        section[SurveyMakerWizard::courseinfo]->hide();
    }
}

void PreviewAndExportPage::cleanupPage()
{
    delete survey;

    // going back to previous page, so allow user to immediately return to this preview
    QList<QWizard::WizardButton> buttonLayout;
    buttonLayout << QWizard::CancelButton << QWizard::Stretch << QWizard::BackButton << QWizard::NextButton << QWizard::CustomButton2;
    wizard()->setButtonLayout(buttonLayout);
    setButtonText(QWizard::CustomButton2, "Return to Preview");
    wizard()->button(QWizard::CustomButton2)->setStyleSheet(NEXTBUTTONSTYLE);
    connect(wizard(), &QWizard::customButtonClicked, this, [this](int customButton)
            {if(customButton == QWizard::CustomButton2) {while(wizard()->currentId() != SurveyMakerWizard::Page::previewexport) {wizard()->next();}}});
}

void PreviewAndExportPage::saveSurvey()
{
    QFileInfo saveFileLocation = (qobject_cast<SurveyMakerWizard *>(wizard()))->saveFileLocation;
    //save all options to a text file
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), saveFileLocation.canonicalFilePath(), tr("gruepr survey File (*.gru);;All Files (*)"));
    if( !(fileName.isEmpty()) )
    {
        QFile saveFile(fileName);
        if(saveFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            saveFileLocation.setFile(QFileInfo(fileName).canonicalPath());
            QJsonObject saveObject;
            saveObject["Title"] = field("SurveyTitle").toString();

            saveObject["FirstName"] = field("FirstName").toBool();
            saveObject["LastName"] = field("LastName").toBool();
            saveObject["Email"] = field("Email").toBool();
            saveObject["Gender"] = field("Gender").toBool();
            saveObject["GenderType"] = field("genderOptions").toInt();
            saveObject["URM"] = field("RaceEthnicity").toBool();

            const int numMultiChoiceQuestions = field("multiChoiceNumQuestions").toInt();
            saveObject["numAttributes"] = numMultiChoiceQuestions;
            QList<QString> multiQuestionTexts = field("multiChoiceQuestionTexts").toStringList();
            auto multiQuestionResponses = field("multiChoiceQuestionResponses").toList();
            auto multiQuestionMultis = field("multiChoiceQuestionMultis").toList();
            auto responseOptions = QString(RESPONSE_OPTIONS).split(';');
            for(int question = 0; question < numMultiChoiceQuestions; question++)
            {
                saveObject["Attribute" + QString::number(question+1)+"Question"] = multiQuestionTexts[question];
                const int responseOptionNum = responseOptions.indexOf(multiQuestionResponses[question].toStringList().join(" / "));
                saveObject["Attribute" + QString::number(question+1)+"Response"] = responseOptionNum + 1;
                saveObject["Attribute" + QString::number(question+1)+"AllowMultiResponse"] = multiQuestionMultis[question].toBool();
                if(responseOptionNum == -1) // custom options being used
                {
                    saveObject["Attribute" + QString::number(question+1)+"Options"] = multiQuestionResponses[question].toStringList().join(" / ");
                }
            }
            saveObject["Schedule"] = field("Schedule").toBool();
            saveObject["ScheduleAsBusy"] = (field("scheduleBusyOrFree").toInt() == SchedulePage::busy);
            saveObject["Timezone"] = field("Timezone").toBool();
            saveObject["baseTimezone"] = field("baseTimezone").toString();
            saveObject["scheduleQuestion"] = field("ScheduleQuestion").toString();
            QList<QString> dayNames = field("scheduleDayNames").toStringList();
            for(int day = 0; day < MAX_DAYS; day++)
            {
                QString dayString1 = "scheduleDay" + QString::number(day+1);
                QString dayString2 = dayString1 + "Name";
                saveObject[dayString1] = !dayNames[day].isEmpty();
                saveObject[dayString2] = dayNames[day];
            }
            saveObject["scheduleStartHour"] = field("scheduleFrom").toInt();
            saveObject["scheduleEndHour"] = field("scheduleTo").toInt();
            saveObject["Section"] = field("Section").toBool();
            saveObject["SectionNames"] = field("SectionNames").toStringList().join(',');
            saveObject["PreferredTeammates"] = field("PrefTeammate").toBool();
            saveObject["PreferredNonTeammates"] = field("PrefNonTeammate").toBool();
            saveObject["numPrefTeammates"] = field("numPrefTeammates").toInt();

            QJsonDocument saveDoc(saveObject);
            saveFile.write(saveDoc.toJson());
            saveFile.close();
        }
        else
        {
            QMessageBox::critical(this, tr("No Files Saved"), tr("This survey was not saved.\nThere was an issue writing the file to disk."));
        }
    }
}

void PreviewAndExportPage::exportSurvey()
{

}
