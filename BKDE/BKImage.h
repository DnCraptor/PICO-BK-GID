#pragma once

#include "BKFloppyImage_Prototype.h"

/*
Класс, где будут все основные методы для работы с образом.
Открытие образа, закрытие,
добавление файлов/директорий (групповое)
удаление файлов/директорий (групповое и рекурсивное)
создание директорий
извлечение файлов и преобразование форматов
*/
enum class ADD_ERROR : int
{
	OK_NOERROR = 0, // нет ошибок
	IMAGE_NOT_OPEN, // файл образа не открыт
	FILE_TOO_LARGE, // файл слишком большой
	USER_CANCEL,    // операция отменена пользователем
	IMAGE_ERROR,    // ошибку смотри в nImageErrorNumber
	NUMBERS
};

extern std::wstring g_AddOpErrorStr[];

struct ADDOP_RESULT
{
	bool            bFatal;     // флаг необходимости прервать работу.
	ADD_ERROR       nError;     // номер ошибки в результате добавления объекта в образ
	IMAGE_ERROR     nImageErrorNumber; // номер ошибки в результате операций с образом
	BKDirDataItem   afr;        // экземпляр абстрактной записи, которая вызвала ошибку.
	// Она нам нужна будет для последующей обработки ошибок
	ADDOP_RESULT()
		: bFatal(false)
		, nError(ADD_ERROR::OK_NOERROR)
		, nImageErrorNumber(IMAGE_ERROR::OK_NOERRORS)
	{
		afr.clear();
	}
};



class CBKListCtrl;

class CBKImage
{
		fs::path                m_strStorePath;     // путь, куда будем сохранять файлы
		CBKListCtrl            *m_pListCtrl;

		PaneInfo                m_PaneInfo;
		std::vector<PaneInfo>   m_vSelItems;

		std::unique_ptr<CBKFloppyImage_Prototype> m_pFloppyImage;
		std::vector<std::unique_ptr<CBKFloppyImage_Prototype>> m_vpImages; // тут будут храниться объекты, когда мы заходим в лог.диски

		bool m_bCheckUseBinStatus;      // состояние чекбоксов "использовать формат бин"
		bool m_bCheckUseLongBinStatus;  // состояние чекбоксов "использовать формат бин"
		bool m_bCheckLogExtractStatus;  // и "создавать лог извлечения" соответственно, проще их тут хранить, чем запрашивать сложными путями у родителя

	protected:
		void StepIntoDir(BKDirDataItem *fr);
		bool StepUptoDir(BKDirDataItem *fr);
		void OutCurrFilePath();

		bool ExtractObject(BKDirDataItem *fr);
		bool ExtractFile(BKDirDataItem *fr);
		bool DeleteRecursive(BKDirDataItem *fr);
		bool AnalyseExportFile(AnalyseFileStruct *a);
		void SetStorePath(const fs::path &str);
		bool ViewFile(BKDirDataItem *fr);
		bool ViewFileRT11(BKDirDataItem *fr);
		bool ViewFileAsSprite(BKDirDataItem *fr);

	public:
		CBKImage();
		~CBKImage();
		void AttachView(CBKListCtrl *pListCtrl)
		{
			m_pListCtrl = pListCtrl;
		}

		uint32_t Open(PARSE_RESULT &pr, const bool bLogDisk = false); // открыть файл по результатам парсинга
		uint32_t ReOpen(); // переинициализация уже открытого образа
		void Close(); // закрыть текущий файл
		const std::wstring GetImgFormatName(IMAGE_TYPE nType = IMAGE_TYPE::UNKNOWN);
		void ClearImgVector();
		void PushCurrentImg();
		bool PopCurrentImg();

		struct ItemPanePos
		{
			int nTopItem;
			int nFocusedItem;
			ItemPanePos() : nTopItem(0), nFocusedItem(0) {}
			ItemPanePos(int t, int f) : nTopItem(t), nFocusedItem(f) {}
		};

		CBKImage::ItemPanePos GetTopItemIndex();

		bool ReadCurrentDir(CBKImage::ItemPanePos pp);

		inline bool IsImageOpen()
		{
			return (m_pFloppyImage != nullptr);
		}

		inline bool GetImageOpenStatus()
		{
			return m_pFloppyImage->GetImageOpenStatus();
		}
		inline unsigned long GetBaseOffset() const
		{
			return m_pFloppyImage->GetBaseOffset();
		}
		inline unsigned long GetImgSize() const
		{
			return m_pFloppyImage->GetImgSize();
		}
		inline void SetCheckBinExtractStatus(bool bStatus = false)
		{
			m_bCheckUseBinStatus = bStatus;
		}
		inline void SetCheckUseLongBinStatus(bool bStatus = false)
		{
			m_bCheckUseLongBinStatus = bStatus;
		}
		inline void SetCheckLogExtractStatus(bool bStatus = false)
		{
			m_bCheckLogExtractStatus = bStatus;
		}
		void ItemProcessing(int nItem, BKDirDataItem *fr);
		void ViewSelected(const bool bViewAsText = true);
		void ExtractSelected(const fs::path &strOutPath);
		void RenameRecord(BKDirDataItem *fr);
		void DeleteSelected();

		// добавить в образ файл/директорию
		ADDOP_RESULT AddObject(const fs::path &findFile, bool bExistDir = false);
		void OutFromDirObject(BKDirDataItem *fr);
		// удалить из образа файл/директорию
		ADDOP_RESULT DeleteObject(BKDirDataItem *fr, bool bForce = false);
};

