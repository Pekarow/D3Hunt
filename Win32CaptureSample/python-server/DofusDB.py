from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.support import expected_conditions as EC
from selenium.webdriver.common.action_chains import ActionChains
import undetected_chromedriver as uc
import re

from time import sleep
import difflib
x_xpath = '/html/body/div[1]/div/div/main/div/div[3]/div[1]/label/div/div[1]/div/input'
y_xpath = '/html/body/div[1]/div/div/main/div/div[3]/div[2]/label/div/div[1]/div/input'
pattern = r'.[\d]+,.[\d]+'
regex = re.compile(pattern)
def clear(elem):
    elem.send_keys(Keys.CONTROL + "a")
    elem.send_keys(Keys.DELETE)
class DofusDB:
    def __init__(self) -> None:
        # Initialization of selenium connection
        chrome_options = uc.ChromeOptions()
        chrome_options.add_argument('--disable-extensions')
        chrome_options.add_argument('--profile-directory=Default')
        chrome_options.add_argument("--incognito")
        chrome_options.add_argument("--disable-plugins-discovery")
        chrome_options.add_argument("--start-maximized")
        chrome_options.add_argument("--headless=new")
        self.driver = uc.Chrome(options=chrome_options)

        self.driver.delete_all_cookies()
        self.driver.get("https://dofusdb.fr/fr/tools/treasure-hunt")
        
    def set_x_position(self, pos: int) -> None:
        # Set position of X coordinate in browser
        
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, x_xpath)))
        clear(elem)
        elem.send_keys(pos)

    def set_y_position(self, pos: int) -> None:
        # Set position of Y coordinate in browser
        
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, y_xpath)))
        clear(elem)
        elem.send_keys(pos)

    def set_direction(self, pos: int) -> None:
        # Set direction in browser
        actions = ActionChains(self.driver)
        
        if pos == 0:
            actions.send_keys(Keys.ARROW_UP)
        elif pos == 1:
            actions.send_keys(Keys.ARROW_RIGHT)
        elif pos == 2:
            actions.send_keys(Keys.ARROW_DOWN)
        elif pos == 3:
            actions.send_keys(Keys.ARROW_LEFT)
        actions.perform()
        sleep(1)

    def set_hint(self, hint: str) -> None:
        # Set hint based on available choices
        hint_xpath = '/html/body/div[1]/div/div/main/div/div[7]/div/label/div/div/div[2]/i'
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.XPATH, hint_xpath)))
        elem.click()
        sleep(1)
        elem = WebDriverWait(self.driver, 30).until(
                 EC.presence_of_element_located((By.CLASS_NAME, "q-menu")))

        elems = elem.find_elements(By.CLASS_NAME, "q-item__label")
        elems_string = [e.text for e in elems]
        print(elems_string)
        closest = difflib.get_close_matches(word=hint, possibilities=elems_string, n=1, cutoff=.8)
        print(closest)
        sleep(1)
        actions = ActionChains(self.driver)
        actions.send_keys(*closest)
        actions.pause(1)
        actions.send_keys(Keys.DOWN)
        actions.send_keys(Keys.ENTER)
        actions.perform()
        

    def get_hint_position(self)->"tuple[str,str]":
        
        elem = WebDriverWait(self.driver, 30).until(
                EC.presence_of_element_located((By.TAG_NAME, "main")))

        results = regex.findall(elem.text, re.DOTALL)
        return tuple(results[0].split(','))

if __name__ == "__main__":
    db = DofusDB()
    db.set_x_position(12)
    db.set_y_position(-63)
    # sleep(4)

    db.set_direction(1)
    # sleep(5)
    db.set_hint("Cactus sans pines")
    sleep(1)
    x, y = db.get_hint_position()
    print(x, y)
    # db.set_x_position(int(x))
    # db.set_y_position(int(y))
    db.driver.close()
    
