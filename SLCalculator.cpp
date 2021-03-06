#include "SLCalculator.h"
#include <QCombobox>
#pragma execution_character_set("utf-8")
POSDATA posData[6];
RESULTPOSDATA resultPos;
QTranslator translator;
void QComboBox::wheelEvent(QWheelEvent* null) {}//重写组合框滚轮事件以禁用滚轮防止误切换语言
MCSC::MCSC(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	MCSC::enumQmFile();
	//默认隐藏翻译按钮
	ui.pb_ctr_1->setVisible(false);
	ui.pb_ctr_2->setVisible(false);
	//信号连接
	connect(ui.pb_start, &QPushButton::clicked, this, &MCSC::startBtnClicked);
	connect(ui.pb_nextInput, &QPushButton::clicked, this, &MCSC::nextInput);
	connect(ui.pb_goCalibration, &QPushButton::clicked, this, &MCSC::goCalibration);
	connect(ui.pb_goResult, &QPushButton::clicked, this, &MCSC::showResult);
	connect(ui.language, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &MCSC::changeLanguage);
	//输入后自动跳转下一个
	connect(ui.throwPos_X, &QSpinBox::editingFinished, this, &MCSC::TX_Next);
	connect(ui.throwPos_Y, &QSpinBox::editingFinished, this, &MCSC::TY_Next);
	connect(ui.landingPos_X, &QSpinBox::editingFinished, this, &MCSC::LX_Next);
	//翻译按钮
	connect(ui.pb_ctr_1, &QPushButton::clicked, this, &MCSC::CTR1);
	connect(ui.pb_ctr_2, &QPushButton::clicked, this, &MCSC::CTR2);
}
void MCSC::enumQmFile(void)
{
	//枚举目录下所有以.qm结尾的文件，加入选择语言列表
	QStringList fileNames;
	QStringList nameFilters;
	nameFilters.append("*.qm");
	QDir dir;
	dir.setPath(QCoreApplication::applicationDirPath());
	fileNames = dir.entryList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);
	int rdpos = 0;
	QFile file;
	while (rdpos + 1 <= fileNames.count())
	{
		ui.language->addItem(fileNames[rdpos]);
		rdpos += 1;
	}
}
void MCSC::changeLanguage(int)
{
	if (ui.language->currentIndex() == 0)//如果选择简体中文
	{
		ui.pb_ctr_1->setVisible(false);
		ui.pb_ctr_2->setVisible(false);
		ui.title_translator->setText("此语言是本程序的源语言");
	}
	else//如果选择载入新语言
	{
		ui.language->setEnabled(false);
		ui.pb_ctr_1->setVisible(true);
		ui.pb_ctr_2->setVisible(true);
		translator.load(QCoreApplication::applicationDirPath() + "/" + ui.language->currentText());
		qApp->installTranslator(&translator);//安装语言
	}
	this->retranslateUi();//重新显示UI
}
void MCSC::startBtnClicked(void)
{
	//开始按钮按下事件，写提示文本
	ui.pb_nextInput->setEnabled(false);
	ui.throwPos_X->setFocus();
	ui.throwPos_X->selectAll();
	ui.stepMenu->setCurrentIndex(1);
	ui.pageInfoTitle_input->setText(tr("记录当前坐标，然后投掷末影之眼。") +
		"\n" + tr("对准末影之眼飞行的方向直行直到碰到障碍，记录最终坐标。") + "\n" +
		tr("请将记录的两组坐标填入下面的框中, 然后点击下一步。"));
	MCSC::nowInputedPos = 0;
}
void MCSC::nextInput(void)
{
	isEditedPos = {};
	ui.pb_nextInput->setEnabled(false);
	ui.throwPos_X->setFocus();
	ui.throwPos_X->selectAll();
	//储存输入的内容
	posData[nowInputedPos].throwPos_X = ui.throwPos_X->value();
	posData[nowInputedPos].throwPos_Y = ui.throwPos_Y->value();
	posData[nowInputedPos].landingPos_X = ui.landingPos_X->value();
	posData[nowInputedPos].landingPos_Y = ui.landingPos_Y->value();
	//重置输入框
	ui.throwPos_X->setValue(0);
	ui.throwPos_Y->setValue(0);
	ui.landingPos_X->setValue(0);
	ui.landingPos_Y->setValue(0);
	//不同输入状态
	MCSC::nowInputedPos += 1;
	if (MCSC::nowInputedPos == 1)//当前已有一组数据
	{
		ui.pageInfoTitle_input->setText(tr("很好！现在离开刚才的位置，随意往左或往右移动几十格。") +
			"\n" + tr("找到一个空旷的地方，然后重复刚才的操作。") +
			"\n" + tr("注意，请保证两次末影之眼指向的是同一个要塞。"));
	}
	if (MCSC::nowInputedPos == 2)//当前已有两组数据
	{
		ui.pageInfoTitle_input->setText(tr("就是那样！现在你已经熟悉应该怎样做了。") +
			"\n" + tr("重复刚才的操作：移动，抛掷，然后记录坐标。") +
			"\n" + tr("同样注意，末影之眼指向的应是同一个要塞。"));
	}
	if (MCSC::nowInputedPos >= 3)//当前已输入完成，或处于校准阶段
	{
		if (MCSC::nowInputedPos >= 5)//校准超过五次阻止下一次校准
		{
			ui.pb_goCalibration->setEnabled(false);
		}
		ui.stepMenu->setCurrentIndex(2);
		resultPos = MCSC::calculateSLPos();
		if (resultPos.errStatistics > 150)
		{
			if (lastErrStatistics == 0)//第一次误差提示
			{
				ui.pageInfoTitle_calibration->setText(tr("误差似乎有点大...") +
					"\n" + tr("不过你现在可以通过重新输入一组数据的方式校准结果")
					+ "\n" + tr("当前估计误差：") + QString::number(round(resultPos.errStatistics)) + tr("格"));
			}
			else if (lastErrStatistics >= resultPos.errStatistics)//校准后误差提示
			{
				ui.pageInfoTitle_calibration->setText(tr("误差似乎减小了，但还是不尽如人意") +
					"\n" + tr("你仍可以通过重新输入一组数据的方式校准结果") +
					"\n" + tr("当前估计误差：") + QString::number(round(resultPos.errStatistics)) + tr("格"));
			}
			else//校准后误差反而变大的情况
			{
				ui.pb_goCalibration->setEnabled(false);
				ui.pageInfoTitle_calibration->setText(tr("我们怀疑您输入的某组数据有误，因此不再提供校准机会") +
					"\n" + tr("我们会给你最终结果作为参考，但其可能有误。") +
					"\n" + tr("当前估计误差：") + QString::number(round(resultPos.errStatistics)) + tr("格"));
			}
			ui.mainPageTitle_calibration->setStyleSheet("background-color: rgb(216, 146, 122)");
		}
		else//误差可接受
		{
			ui.pageInfoTitle_calibration->setText(tr("好消息，现在误差并不是很大！") +
				"\n" + tr("如果你觉得误差能接受的话，现在就可以获取结果了") +
				"\n" + tr("当前估计误差：") + QString::number(round(resultPos.errStatistics)) + tr("格"));
			ui.mainPageTitle_calibration->setStyleSheet("background-color: rgb(155, 183, 212)");
		}
		lastErrStatistics = resultPos.errStatistics;
	}
}
RESULTPOSDATA MCSC::calculateSLPos(void)
{
	UINT8 posDataNum1 = 0;
	UINT8 posDataNum2 = 0;
	UINT8 IntersectionDataNum = 0;
	INTERSECTIONPOSDATA IntersectionPosData[21] = {};//存储焦点坐标,零号位存储焦点坐标总和
	RESULTPOSDATA resultPosData;

	while (posDataNum1 <= MCSC::nowInputedPos - 1)//握手模型枚举调用计算函数
	{
		posDataNum2 = posDataNum1 + 1;
		while (posDataNum2 <= MCSC::nowInputedPos - 1)
		{
			IntersectionDataNum += 1;
			IntersectionPosData[IntersectionDataNum] = MCSC::findIntersectionPos(posDataNum1, posDataNum2);//计算并储存数据
			posDataNum2 += 1;
		}
		posDataNum1 += 1;
	}
	UINT8 nowInPosData = 0;
	//求平均
	while (nowInPosData <= IntersectionDataNum)//求多点的中点，减小误差
	{
		nowInPosData += 1;
		IntersectionPosData[0].X += IntersectionPosData[nowInPosData].X;
		IntersectionPosData[0].Y += IntersectionPosData[nowInPosData].Y;
	}
	resultPosData.X = IntersectionPosData[0].X / IntersectionDataNum;
	resultPosData.Y = IntersectionPosData[0].Y / IntersectionDataNum;
	//误差计算
	nowInPosData = 0;
	UINT32 es_X = 0;
	UINT32 es_Y = 0;
	while (nowInPosData <= IntersectionDataNum - 1)
	{
		nowInPosData += 1;
		qDebug() << abs(IntersectionPosData[nowInPosData].X - resultPosData.X) << ":" << IntersectionPosData[nowInPosData].X << "-" << resultPosData.X;
		es_X += abs(IntersectionPosData[nowInPosData].X - resultPosData.X);
		es_Y += abs(IntersectionPosData[nowInPosData].Y - resultPosData.Y);
	}
	es_X = es_X / IntersectionDataNum;
	es_Y = es_Y / IntersectionDataNum;
	resultPosData.errStatistics = (sqrt(pow(es_X, 2) + pow(es_Y, 2)))/2;//勾股求斜边误差
	return resultPosData;
}
INTERSECTIONPOSDATA MCSC::findIntersectionPos(UINT8 parPosData1_Num, UINT8 parPosData2_Num)
{
	//通过坐标列出解析式(y=kx+b)
	INTERSECTIONPOSDATA result;
	float k1 = (posData[parPosData1_Num].landingPos_Y - posData[parPosData1_Num].throwPos_Y) / (posData[parPosData1_Num].landingPos_X - posData[parPosData1_Num].throwPos_X);
	float b1 = posData[parPosData1_Num].throwPos_Y - (posData[parPosData1_Num].throwPos_X * k1);
	float k2 = (posData[parPosData2_Num].landingPos_Y - posData[parPosData2_Num].throwPos_Y) / (posData[parPosData2_Num].landingPos_X - posData[parPosData2_Num].throwPos_X);
	float b2 = posData[parPosData2_Num].throwPos_Y - (posData[parPosData2_Num].throwPos_X * k2);
	//求解二元一次方程组得出焦点坐标
	result.X = (b1 - b2) / (k2 - k1);
	result.Y = k1 * result.X + b1;
	result.parPosData1_Num = parPosData1_Num;
	result.parPosData2_Num = parPosData2_Num;
	return result;
}
void MCSC::goCalibration(void)
{
	//校准事件
	ui.pageInfoTitle_input->setText(tr("校准数据"));
	ui.pageInfoTitle_input->setText(tr("还是你熟悉的操作：移动，抛掷，然后记录坐标。") +
		"\n" + tr("同样注意，末影之眼指向的应是同一个要塞。"));
	ui.stepMenu->setCurrentIndex(1);
}
void MCSC::showResult(void)
{
	//结果显示
	ui.stepMenu->setCurrentIndex(3);
	ui.pageInfoTitle_result->setText(tr("可能的要塞坐标：(") + QString::number(resultPos.X) + "," +
		QString::number(resultPos.Y) + ")\n" + tr("估计误差：") + QString::number(round(resultPos.errStatistics)) + tr("格"));
	if (resultPos.errStatistics > 150)
	{
		ui.mainPageTitle_result->setText(tr("  参考结果"));
		ui.mainPageTitle_result->setStyleSheet("background-color: rgb(216, 146, 122)");
	}
	else
	{
		ui.mainPageTitle_result->setText(tr("  结果"));
		ui.mainPageTitle_result->setStyleSheet("background-color: rgb(155, 183, 212)");
	}
}
void MCSC::retranslateUi(void)
{
	//重新加载UI的文字使翻译生效
	ui.title_translator->setText(tr("Put your name there :)"));
	ui.pb_goCalibration->setText(tr("校准"));
	ui.pb_goResult->setText(tr("查看结果"));
	ui.pb_nextInput->setText(tr("下一步"));
	ui.pb_start->setText(tr("开始"));
	ui.mainPageTitle_start->setText(tr("  开始"));
	ui.mainPageTitle_input->setText(tr("  输入坐标"));
	ui.title_input_1->setText(tr("投掷时坐标"));
	ui.title_input_2->setText(tr("延线终坐标"));
	this->setWindowTitle(tr("Minecraft要塞坐标计算器"));
	ui.pageInfoTitle_start->setText(tr("你需要投掷末影之眼，然后按要求记录投掷时的坐标与\n其飞行路径水平延伸线尽头的坐标。\n请尽可能保证坐标准确。\n另外，你需要保证所有末影之眼均指向同一座要塞。\n准备好三个以上末影之眼，然后开始吧！"));
}